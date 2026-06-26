# Testing and examples notes


## Examples

 * File shared: Usando ```MAP_SHARED```, viene mappata nell'address space del processo la pagina "originale" della page cache (e non la copia, per esempio con ```MAP_PRIVATE```). 

Adesso, dato che sostanzialmente lo stato della page cache non dipende da alcun processo, ma è il kernel che valuta se, quando e perchè liberare la memoria (e.g. memory pressure elevata, o anche solo per richiesta dell'utente), anche quando il processo example termina, e il folio in page cache relativo ai dati del file ha un decremento del refcount, è possibile che quel folio rimanga li per molto tempo "indisturbato".

Supponiamo che stiamo eseguendo il processo example per la prima volta (la page cache *non* contiene il folio del file mappato con ```MAP_SHARED```): l'esempio va a buon fine. Rieseguendo subito dopo l'esempio, fallisce.

Questo perchè la "macchina a stati" tiene traccia della page/folio (in realtà l'indice è il pfn) fino alla sua liberazione, cosa che dato il poco lasso di tempo tra i due esempi, non è avvenuta.

 Dato che entrambe le istanze di processo usano lo stesso frame, la macchina a stati sa già che il frame è stato "referenziato" WX in passato da una o più PTEs, causando fallimento del test/esempio:
Nel senso, il comportamento sarebbe corretto, solo che per come funziona la libreria "libscid" fa una ```recv()``` da un netlink socket che avvisa quando viene rilevato un wx su una pagina e fa un confronto per capire se l'evento è corretto

Dato che il frame è ancora presente in RAM (con lo shellcode iniettato dall'adversary), la macchina a stati non lo determina come nuovo evento, ma sa che il frame è sporco

Quindi la recv, che è bloccante, rimane in attesa senza mai ricevere nulla

   - In questo primo caso, prima esecuzione (partiamo da page cache vuota) e la seconda che non va a buon fine (ho fatto CTRL+C perchè il poll dei messaggi andrebbe avanti all'infty, dato che non arriva la segnalazione wxwarning)

   - In questo secondo caso, invece, ho richiesto al kernel di liberare la page cache. In definitiva, negli esempi sarà sufficiente includere questi comandi... (con ```flush_page_cache```)

 * Test per quanto riguarda ```mremap``` e ```mlock``` ing. ```mremap``` manipola l'address space del processo e "non tocca" la memoria fisica, alla fin fine, viene sempre puntata quella, ma da altre PTEs.

  - Attenzione a ```MREMAP_DONTUNMAP```: il mapping rimane valido (il kernel lo "riconosce" come valido. VMA valide.) ma le hw ptes del vecchio mapping **non** puntano più alla vecchia area di memoria
  (quindi, **non** esistono due PTEs appartenenti allo stesso processo che puntano alla stessa memoria fisica)
  ma ripartono da zero (page fault, zeropage, ...) puntando a nuova memoria in modo trasparente, e non causando SIGSEGV (senza il DONTUNMAP, il range di indirizzi virtuali vecchio non ha piu un VMA descriptor 
  associato valido e quindi SIGSEGV a seguito del page fault al tentativo di accessoo)

  - ```mlockall``` con ```MCL_FUTURE```: per via della logica di ```mlock```, al ritorno dalla syscall all'utente le pagine sono presenti e locked in memoria (VMA ha flag tipo ```VM_LOCKED```). 
  ```mlockall``` con ```MCL_FUTURE``` causa il prefault di tutte le pagine successivamente mappate (e.g. ```mmap```) perchè è necessario che la pagina sia già presente per "bloccarla" in memoria 
  (niente più demand paging o cose lazy/deferred).

 * Qualche esempio con prefaulting, ampiamente testato. 
 In generale, sia con ```MAP_POPULATE``` che ```madvise``` si arriva, passando per ```get_user_pages``` (gup), a
 ```handle_mm_fault``` e quindi ```handle_pte_fault```, simulando un page fault software anticipato, quindi gli hook intercettano.
