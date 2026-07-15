# Testing and examples notes


## Examples

 * **File shared: Usando ```MAP_SHARED```, viene mappata nell'address space del processo la pagina "originale" della page cache (e non la copia, per esempio con ```MAP_PRIVATE```).**

Adesso, dato che sostanzialmente lo stato della page cache non dipende da alcun processo, ma è il kernel che valuta se, quando e perchè liberare la memoria (e.g. memory pressure elevata, o anche solo per richiesta dell'utente), anche quando il processo example termina, e il folio in page cache relativo ai dati del file ha un decremento del refcount, è possibile che quel folio rimanga li per molto tempo "indisturbato".

Supponiamo che stiamo eseguendo il processo example per la prima volta (la page cache *non* contiene il folio del file mappato con ```MAP_SHARED```): l'esempio va a buon fine. Rieseguendo subito dopo l'esempio, fallisce.

Questo perchè la "macchina a stati" tiene traccia della page/folio (in realtà l'indice è il pfn) fino alla sua liberazione, cosa che dato il poco lasso di tempo tra i due esempi, non è avvenuta.

 Dato che entrambe le istanze di processo usano lo stesso frame, la macchina a stati sa già che il frame è stato "referenziato" WX in passato da una o più PTEs, causando fallimento del test/esempio:
Nel senso, il comportamento sarebbe corretto, solo che per come funziona la libreria "libscid" fa una ```recv()``` da un netlink socket che avvisa quando viene rilevato un wx su una pagina e fa un confronto per capire se l'evento è corretto

Dato che il frame è ancora presente in RAM (con lo shellcode iniettato dall'adversary), la macchina a stati non lo determina come nuovo evento, ma sa che il frame è sporco

Quindi la recv, che è bloccante, rimane in attesa senza mai ricevere nulla.

**Cosa ho fatto**

*Prima esecuzione (partiamo da page cache vuota) e la seconda che non va a buon fine (ho fatto CTRL+C perchè il poll dei messaggi andrebbe avanti all'infty, dato che non arriva la segnalazione wxwarning).*

*In secondo caso, invece, ho richiesto al kernel di liberare la page cache tra un'esecuzione e l'altra. In definitiva, negli esempi sarà sufficiente includere questi comandi... (con ```flush_page_cache```)*

 * **Test per quanto riguarda ```mremap``` e ```mlock``` ing. ```mremap``` manipola l'address space del processo e "non tocca" la memoria fisica, alla fin fine, viene sempre puntata quella, ma da altre PTEs.**

   - Attenzione a ```MREMAP_DONTUNMAP```: il mapping rimane valido (il kernel lo "riconosce" come valido. VMA valide.) ma le hw ptes del vecchio mapping **non** puntano più alla vecchia area di memoria
  (quindi, **non** esistono due PTEs appartenenti allo stesso processo che puntano alla stessa memoria fisica)
  ma ripartono da zero (page fault, zeropage, ...) puntando a nuova memoria in modo trasparente, e non causando SIGSEGV (senza il DONTUNMAP, il range di indirizzi virtuali vecchio non ha piu un VMA descriptor 
  associato valido e quindi SIGSEGV a seguito del page fault al tentativo di accessoo)

   - ```mlockall``` con ```MCL_FUTURE```: per via della logica di ```mlock```, al ritorno dalla syscall all'utente le pagine sono presenti e locked in memoria (VMA ha flag tipo ```VM_LOCKED```). 
  ```mlockall``` con ```MCL_FUTURE``` causa il prefault di tutte le pagine successivamente mappate (e.g. ```mmap```) perchè è necessario che la pagina sia già presente per "bloccarla" in memoria 
  (niente più demand paging o cose lazy/deferred).

 * **Qualche esempio con prefaulting, ampiamente testato.**

 In generale, sia con ```MAP_POPULATE``` che ```madvise``` si arriva, passando per ```get_user_pages``` (gup), a
 ```handle_mm_fault``` e quindi ```handle_pte_fault```, simulando un page fault software anticipato, quindi gli hook intercettano.

 * **```brk/sbrk```** e SELinux: non faccio l'esempio perchè SELinux (o altri MAC) potrebbero impedire ```PROT_EXEC``` (tramite i security hooks):
   - ```security_file_mprotect``` chiamato da ```do_mprotect_pkey``` (https://elixir.bootlin.com/linux/v7.1.2/source/mm/mprotect.c#L953)
   - ```selinux_file_mprotect``` impl LSM, vedere controllo EXEC su start_brk, brk (https://elixir.bootlin.com/linux/v7.1.2/source/security/selinux/hooks.c#L4098)
   - in sintesi, ```mprotect(*X)``` su area anonima privata espansa con ```brk/sbrk``` da "Permesso negato"
   - Per la memoria anonima, il security hook ```file_mprotect``` ha ptr a file NULL

 * Perchè negli esempi gli snapshot confrontati con libscid sono il "pre_op_buffer" (il frame prima dell'op nell'esempio) per fault di tipo write e 
 il frame "attuale" (_va) negli altri casi (no-fault o exec-fault che non succede mai)? Perchè lo snapshot viene fatto all'accadimento del fault di tipo write:
 la write non può essere effettuata se non dopo la gestione del page fault da parte del sistema operativo, e il ritorno del controllo allo userspace con la hw pte fixata,
 quindi il frame snapshottato è quello **prima** della write op. Potremmo semplificare la logica e usare sempre "pre_op_buffer" come buffer di confronto (le read ops o le exec ops
 non modificano il frame, quindi rimane invariato prima e dopo la op), ma facciamo cosi per coerenza. Il discorso vale ovviamente sempre (per ogni snapshot).
 
 * Perchè negli esempi (in particolare quelli wxwarning) non si vede mai il fault exec? Perchè l'exec è sempre abilitato (altrimenti avremmo SIGSEGV, non ha senso...). 
 Il "fault exec" arà visibile con esempi di snapshot non-after-wxwarn (ovvero, non iniziali), dove la PTE avrà effettivamente exec bit disabilitato 
 (o no-exec abilitato, stessa cosa...) e quindi si cattura lo snapshot grazie al fault di instruction fetch ("exec")

 * **IN GENERALE QUINDI, NON E' POSSIBILE LEGGERE/FARE SNAPSHOT DEL FRAME DOPO LA WRITE ATTUALE, OVVERO QUELLA CHE CAUSA IL FAULT, ma sempre la versione del frame PRECEDENTE a quella
 write. (è inevitabile che sia cosi, vedere fuzionamento del page fault handler: il kernel ritorna controllo a userspace dopo che ha settato bene la PTE e quindi user riprova a fare 
 la write, ovviamente in modo del tutto trasparente e grazie al supporto dell'hardware)**

   - La cosa positiva è l'alternanza però: se al write disabilito la exec delle altre PTE, catturerò la exec e vedo il contenuto del frame scritto nella write precedente (effettuata
   concretamente dopo il fault)

 * Vedi problema risolto delle memory protection keys (vedi file docs/10 dedicato) e gli esempi. **NOTA: non sono sicuro che sia proprio questo il
 problema negli esempi, perchè l'errore è 0x4 e quindi il bit PK è disabilitato,
 ma anche il bit di presenza della pagina: read su PTE non presente, forse
 meccanismo software per protezione simile da certe vm_operations_struct??**

## Testing

 * **Soft fail/hard fail**

 * **Prefault con i file**: test disabilitati, mmap potrebbe fallire silenziosamente (non popolare le PTE early)

 * **Per testare l'unmapping/freeing delle pagine** si utilizza una metodologia più particolare a causa del funzionamento del processo di liberazione delle pagine. Vedere nei docs precedenti (05-unmapping.md):
 il kernel effettua lo zapping della PTE, ma il folio "associato" può non essere ritornato al buddy allocator immediatamente: per riassumere, a causa di ```struct cpu_fbatches``` che tiene il refcount > 0
 anche quando, ad esempio, il mapping era ```MAP_ANONYMOUS | MAP_PRIVATE```, bisogna attendere che un secondo thread effettui un'altra operazione di put sul refcount, e quindi chiami ```free_unref_folios```.
 Il problema per i test di unità è che si basano sul thread corrente "in sincronia". La soluzione è stata quella di: collezionare i folio da liberare (tramite ```free_pages_and_swap_cache```, 
 chiamata sempre da ```sys_munmap```) associati
 al thread corrente, inviare ```SIGSTOP``` a quest'ultimo e attendere che venga eseguito ```free_unref_folios```, o da lui stesso (sullo stesso control path, questo è possibile), o da altro thread su quei
 folio. Quando tutti i folio che hanno causato il ```SIGSTOP``` stanno per essere liberati, invia ```SIGCONT``` a quel thread (può farlo anche il thread ```current``` stesso). Il test è quindi basato sul
 tempo e dipende dal workload del sistema. Eventualmente si può tentare di "stimolare" il kernel a liberare questi folio magari eseguendo qualche comando dalla shell, l'importante è che 
 ```free_unref_folios``` venga, prima o poi, utilizzato.

 * Per i file, la page cache ha impatto sul tempo notevole su tempo esecuzione 
 tests: bisogna attendere che il sistema liberi il frame in page cache 
 *(il kernel non ha un reale motivo di farlo se non c'è memory pressure, 
 al contrario di pagine anonime e/o memoria condivisa, il contenuto del file
 può essere sempre "meaningful" e utile, non ha senso quindi svuotare la page
 cache se la memoria non è sotto pressione, anche se non c'è nessun reale
 utilizzatore del folio, potrebbe esserci un consultatore del file in futuro)* 
 oppure provare a "stimolarlo" usando 
 ```sync; echo 1 > /proc/sys/vm/drop_caches``` ripetutamente 
 per far progredire il test. I test relativi all'hook
 ```free_unref_folios```, in particolare verso i file (file-shared e
 file-private) vengono commentati.

 * Per i test su ```free_unref_folios``` hook, la key "entry" è inutilizzata in realtà. E' zero se ```free_unref_folios``` non arriva a essere invocato dallo stesso kernel control path che fa ```munmap```,
 fuf viene eseguito da altro thread (per forza, il thread corrente è stopped). Se e maggiore di zero, significa che ```free_unref_folios``` è stata invocata direttamente dal thread current in maniera 
 sincrona, e quindi le pagine fisiche liberate immediatamente dallo stesso kcp. Il thread di test non è mai andato in SIGSTOP. Di fatto, entry utilizzata per capire se il test sta cercando di testare
 l'unmapping o meno (vedere il codice).
