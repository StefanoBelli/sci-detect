# addhooks

## ```handle_pte_fault```

Utilizzato solamente per tenere traccia del kernel control path (e.g. se una 
qualsiasi delle funzioni successive venisse chiamata da un'altra funzione),
si memorizza la ```struct vm_fault```, e confrontata dagli altri hook, per 
determinare se il kcp è corretto (dato che gli hook possono essere chiamati 
anche da altre funzioni)

Chiamata per gestire le PTEs, user faults, dal page fault handler (arch-dependent):
 
 1. ```exc_page_fault``` (x86, idtentry)
 2. ```handle_page_fault``` (x86)
 3. ```do_user_addr_fault``` (x86)
 4. ```handle_mm_fault```
 5. ```__handle_mm_fault```
 6. **```handle_pte_fault```**

```struct vm_fault``` è utilizzato anche per recuperare informazioni sulla 
gestione del page fault, come la ptep (```vmf->pte``` è il risultato della gestione 
del page fault), il page table lock, 
il contenuto della pte originale, la vma.

Dal kernel 6.5.x ad oggi, la struttura è rimasta pressochè invariata.

 * **Nel primo blocco** si determina se la PTE per il "faulty VA" 
 esiste già (```struct vm_fault::orig_pte```) oppure no:
   * La sua PMD non esiste (settata a 0)
   * La sua PMD esiste ma la suddetta PTE (```orig_pte```) è settata a 0

   Se si verifica uno dei due casi, ```vmf->pte = NULL```

 * **Nel secondo blocco** determiniamo se la ```vmf->pte = NULL```, in tal caso, eseguiamo ```do_pte_missing``` (e poi ```do_anonymous_page``` o ```do_fault```...), fine.

 * **Nel terzo blocco** se ci arriviamo, significa che la ```orig_pte``` ha
 un contenuto valido:
   * La pagina può essere swapped-out (la PTE è non-present e ha informazioni specifiche sullo swapping)
   * NUMA stuff...

   Fine.

 * **Nel quarto blocco** si prende uno spinlock sulla page table lock:
   * si determina se la ```orig_pte``` (letta all'inizio dereferenziando 
   ```vmf->pte```) è ancora uguale all' ```vmf->pte``` attuale, **dopo aver preso il ptl**. Se sono diverse, fai cose col tlb e rilascia il ptl, fine.

   * Se sono ancora uguali, nessuno ha cambiato ```vmf->pte```, quindi procedi: se il fault è di tipo write (```FAULT_FLAG_WRITE```) e la PTE
   non ha i permessi di scrittura, chiama ```do_wp_page```, fine.

 * **Nel quinto blocco** se la condizione precedente non è vera, allora setta solamente i dirty/accessed bits della PTE, fine. Potrebbe anche trattarsi di
 page fault spurious.

 Ovviamente, rilascia il page table lock.

##### ```vmfs``` tiene traccia delle ```struct vm_fault``` usate per il kcp

E' una hashtable (BITS=5) per-CPU, fa fronte a vari problemi:
 * concorrenza dei page fault handler (essendo in process-context) su stessa CPU
 * migrazione thread corrente da CPU a un'altra (rwlock necessario per far
 riflettere la migrazione da una ht per-cpu a un'altra)
 * problemi legati al cpu hotplug evitati usando ```cpu_possible_mask```
 * se un sistema è UP (```!CONFIG_SMP```) il codice è semplificato
 * le funzioni che gli hook utilizzano sono ```add_vmf```, ```del_vmf``` (queste in particolare, solo ```handle_pte_fault```) e ```got_this_vmf``` (tutti gli altri o quasi)

## ```do_anonymous_page```

Viene chiamata quando la PTE è missing: demand paging.

 * **Nel primo blocco**: viene controllato che ```vma->vm_flags``` non è ```VM_SHARED``` perchè per determinare se chiamare o meno ```do_anonymous_page``` si controlla che le vmops siano ```NULL```, infatti per l'anonymous shared mapping si utilizza ```&shmem_anon_vm_ops```, poi si
 effettua un ```pte_alloc``` e si controlla che l'allocazione sia avvenuta.

 * **Nel secondo blocco**: viene controllato che questo primo accesso (ricorda da dove viene chiamata la funzione) non sia una read, in tal caso, 
 fai il setup della PTE per puntare alla zero page, in RO. Negli hooks, questa situazione viene ignorata nell'entry handler di do_anonymous_page (```vmf->flags & FAULT_FLAG_WRITE```).

 * **Nel terzo blocco**: se si arriva qui, significa che il primo accesso è
 una write, quindi alloca il folio di una pagina, crea la PTE opportunamente,
 setta la singola PTE.

 Se il valore di ritorno non è 0 allora la PTE non viene settata ed è un
 errore (```VM_FAULT_SIGBUS```, ```VM_FAULT_OOM```, ...).

 Se ret = 0 invece:
  * ```vmf->pte != NULL``` e settata
  * ```vmf->pte == NULL```
  * ```vmf->pte != NULL``` e non settata (```vmf_pte_changed```)

 Negli hooks viene determinato se ```vmf->pte == NULL```, il resto non dovrebbe
 essere un problema

 **Mi aspetto che una e una sola pagina/PTE sia coinvolta**, perchè il fault avviene su quella specifica pagina, e non è un file (anonima).
 Il nuovo folio della nuova pagina è tale che folio_nr_pages = 1, viene controllato attivamente.

## ```do_wp_page```

Viene chiamato quando si verifica un write fault su una PTE esistente in read-only
(e.g. CoW, ma non solo)

L'hook su ```do_wp_page``` è quasi una noop, nel senso che in realtà è piazzato 
per coprire ```wp_page_reuse```, che non può essere probato (è inlined) che viene chiamato da ```wp_page_shared```, 
```wp_pfn_shared``` e anche direttamente da ```do_wp_page```.

 * **Il primo blocco**: ignorato, riguarda userfaultfd
 * **Nel secondo blocco**: si recupera la pagina dalla orig_pte
 * **Nel terzo blocco**: si verifica se la vma è shared -> chiamata a ```wp_page_reuse```
 * **Nel quarto blocco**: si verifica se il folio/pagina è anon, exclusive -> chiamata a ```wp_page_reuse```
 * **Nel quinto blocco**: situazione peggiore, copy-on-write necessario,
 ovviamente gestisce anche le zero-pages

Mentre ```wp_page_copy``` è gestito da un hook apposito, per ```wp_page_reuse```, la cui descrizione è fornita dai commenti nel sorgente del kernel: 
~~~
 * This can happen either due to the mapping being with the VM_SHARED flag,
 * or due to us being the last reference standing to the page. In either
 * case, all we need to do here is to mark the page as writable and update
 * any related book-keeping
~~~
... bisogna gestirla al ritorno da ```do_wp_page``` tentando di ricostruire
il percorso effettuato da quest'ultima (nell'handler di ritorno).

Oltrettuto, ```wp_page_reuse``` è chiamata da anche da ```finish_mkwrite_fault```, che a sua volta è chiamata da ```wp_pfn_shared``` e ```wp_page_shared``` quando ```vma->vm_ops != NULL e vma->vm_ops->pfn_mkwrite (->page_mkwrite) != NULL```.

**Con ```wp_page_reuse``` mi aspetto che la pagina/PTE coinvolta sia una soltanto**, NON CONTROLLO IL FOLIO.

Nell'hook, in wpr.c, utilizzo ```add_one_page``` e non controllo il folio perchè ```wp_page_reuse``` riutilizza la pagina esistente modificando la PTE (es. aggiunge perm. write),
ovvero, il folio è già esistente e non so se la pagina coinvolta fa parte di un folio con nr_pages > 1 o no, ma controllare folio_nr_pages > 1 potrebbe dare problemi quando non ce ne sono.
Non è detto che tutte le pagine (contigue a livello fisico) in un folio siano mappate sulla stessa PTE (contigue a livello virtuale).

Dato che ```finish_mkwrite_fault``` è (indirettamente) chiamata da ```do_wp_page```, nell'hook (in particolare, nell'handler di ritorno di ```do_wp_page```) se ne tiene conto: viene guardato il campo entry->private:
 * se è 1 allora è stata eseguita ```wp_page_copy```, il return handler di
 ```do_wp_page``` termina immediatamente perchè non ha nulla da fare (niente esecuzione di ```wp_page_reuse```).

 * se è 2 allora è stata eseguita ```finish_mkwrite_fault``` e quindi sicuramente (a meno di errori, valutando il return value) ```wp_page_reuse```: gestire la situazione. Il valore di ritorno va guardato
 tenendo in considerazione che è "mixato" (ORed) e non è detto che se ret != 0 allora
 non c'è successo, ad esempio se ret != 0 e ret & VM_FAULT_COMPLETED != 0

## ```finish_mkwrite_fault```

L'hook di ```finish_mkwrite_fault``` ha il solo scopo di avvertire l' "outer" hook di ```do_wp_page``` che è stata chiamata. La funzione essenzialmente
ritorna ```VM_FAULT_NOPAGE``` (considerata come errore dai chiamanti) o 0. Se ritorna 0, allora ```wp_page_reuse``` viene eseguito.

## ```wp_page_copy```

Attua il meccanismo di copy-on-write.

Essenzialmente fa le seguenti cose:

 * **Primo passo**: alloca il nuovo folio (1 pagina) e determina se 
 ```orig_pte``` punta a una zero-page

 * **Secondo passo**: se non era una zero-page, allora è necessario copiare
 il contenuto del vecchio ```vmf->page``` nella pagina del nuovo folio (```new_folio->page```)

 * **Terzo passo**: acquisisci il page table lock e il puntatore alla pte (```vmf->pte```) da modificare opportunamente per il CoW. Ricontrolla che
il valore di ```vmf->pte``` letta dopo l'acquisizione del ptl è uguale ancora al valore letto inizialmente, ```vmf->orig_pte```

   * **Quarto passo**: incrementa contatori di utilizzo ```mm```

   * **Quinto passo**: crea una nuova PTE, differisce dalla vecchia ```orig_pte``` perchè è RW e punta alla nuova pagina del nuovo folio

   * **Sesto passo**: cose relative a TLB, LRU, rmapping,...

   * **Settimo passo**: Setta la nuova PTE effettivamente (ovviamente, per lo stesso "faulty VA")

   * **Ottavo passo**: Rimozione della rmap per il vecchio folio

 * **Quarto passo alt.** se la condizione scritta nel terzo passo non è vera, esegui codice arch-dependent per forzare aggiornamento per la MMU/TLB -- qualcun'altro ha aggiornato la PTE

 * In ogni caso, **rilascia il page table lock**

 Il valore di ritorno:

  * se è 0, tutto ok CoW ok
  * se non è 0 allora CoW è fallito
  * Non sembrano esserci situazioni in cui se non è 0 allora può anche darsi che sia comunque andato a buon fine.
  * Potrebbe darsi che se è 0 allora CoW è fallito.

Nel codice si controlla che la ```vmf->pte``` al ritorno (return handler di wpc) sia diversa da ```vmf->orig_pte``` e che abbia effettivamente il write flag abilitato.

**Mi aspetto che la pagina coinvolta sia una soltanto**, il kernel non ha motivo di copiare altre pagine se non strettamente necessario.

Viene creato un nuovo folio e quindi controllo che il nuovo folio abbia nr_pages = 1.

## ```do_fault```

Ritornando indietro, questa è l'alternativa a ```do_anonymous_page``` quando la pte non esiste, solo che viene chiamata quando le vmops sono presenti.

Essenzialmente hooked con una kprobe e viene utilizzata per indicare che è stato preso un certo kcp, per tracciare ```set_pte_range``` tramite una bitmap

 * **Controlla che il metodo ```->fault()```** delle vm_ops associate alla vma sia presente

 * **Se questo è il caso, chiama**, in base alla situazione, una delle seguenti tre, che hanno una struttura simile:
  
   * ```do_read_fault``` se si tratta di un fault in lettura iniziale di memoria non anonima (attenzione al fault around)
   * ```do_cow_fault``` se si tratta di un fault in scrittura di memoria non anonima e non condivisa (e.g. file con ```MAP_PRIVATE```)
   * ```do_shared_fault``` se si tratta di un fault in scrittura di memoria non anonima e condivisa (e.g. file con ```MAP_SHARED```)

   Tutte queste funzioni seguono una logica simile:
      * si verifica se si può chiamare ```->fault```
	  * si chiama ```->fault```
	  * alla fine, si chiama ```finish_fault``` per settare le page table con
	  ```set_pte_range```

## ```finish_fault```

L'hook permette di tracciare il kcp corretto prima di chiamare
```set_pte_range``` che potrebbe essere chiamato da diverse parti del kernel.

L'hook setta un bit in una bitmap per tracciare il kcp.

## ```filemap_map_pages```

Spesso settato come ```->map_pages```, utilizzato per tracciare l'utilizzo di 
```set_pte_range```, che viene utilizzato in ```filemap_map_order0_folio``` e 
```filemap_map_folio_range```. Non li ho hookati perchè non hookabili, li avrei potuti utilizzare per aumentare precisione kcp (?) 

Nel nostro contesto, è utilizzato per attuare, 
dentro ```do_read_fault``` il meccanismo del *fault around* per ottimizzare
le prestazioni (ridurre il numero di page fault, roba che dovrebbe essere legata al readahead).

L'hook setta un bit in una bitmap per tracciare il kcp.

## ```set_pte_range```

In maniera simile con quanto fatto per ```do_wp_page```, nell'entry handler tracciamo il kernel control path: può essere eseguito il return handler solo se il chiamante è ```do_fault``` e ```finish_fault``` o ```do_fault``` e ```filemap_map_pages```.

Facciamo questo controllando una bitmap. La differenza con ```do_wp_page``` 
è che in quest'ultima, i controlli di questo tipo vengono fatte su funzioni
che lei stessa chiama (e.g. ```wp_page_copy```) e **non da cui** è chiamata.

In ogni caso, se il return handler può essere chiamato, gli vengono inoltrati
i parametri di ingresso di ```set_pte_range```: ```struct vm_fault *vmf``` e 
```unsigned long nr```, usati per iterare, al ritorno di ```set_pte_range``` sulle PTEs settate da questa.

In questo caso, **mi aspetto che il numero di pagine coinvolte possa essere > 1**, in generale per via del meccanismo di fault-around, memoria non anonima (file), readahead, ...

Prendo il numero di PT entries da controllare dal parametro "nr" di ```set_pte_range```, niente folio coinvolti. La funzione non restituisce nulla, quindi controllo
un range di ptes da ptep fino a ptep + nr.
