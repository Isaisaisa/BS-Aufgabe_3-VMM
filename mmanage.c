/* Description: Memory Manager BSP3*/
/* Prof. Dr. Wolfgang Fohl, HAW Hamburg */
/* Winter 2010/2011
 * 
 * This is the memory manager process that
 * works together with the vmaccess process to
 * mimic virtual memory management.
 *
 * The memory manager process will be invoked
 * via a SIGUSR1 signal. It maintains the page table
 * and provides the data pages in shared memory
 *
 * This process is initiating the shared memory, so
 * it has to be started prior to the vmaccess process
 *
 * TODO:
 * currently nothing
 * */

#include "mmanage.h"

struct vmem_struct *vmem = NULL; //Pointer auf die Struktur vmem_struct. (vgl mit Instanz von einer Klasse)
FILE *pagefile = NULL;
FILE *logfile = NULL;
int signal_number = 0;          /* Received signal */
int Pagefault_no = 0;

/* Zähler für den Index im physikalischen Speicher, 
 * der bis zur maximalen Indexgröße des Speichers hochgezählt wird.
 * Ist also speicher noch frei kann immer in den nächsten Index die Page
 * eingetragen werden*/
int index_counter = 0; 

int
main(void)
{
    struct sigaction sigact;

    /* Init pagefile */
    init_pagefile(MMANAGE_PFNAME);
    if(!pagefile) {
        perror("Error creating pagefile");
        exit(EXIT_FAILURE);
    }

    /* Open logfile */
    logfile = fopen(MMANAGE_LOGFNAME, "w");
    if(!logfile) {
        perror("Error creating logfile");
        exit(EXIT_FAILURE);
    }

    /* Create shared memory and init vmem structure */
    vmem_init();
    if(!vmem) {
        perror("Error initialising vmem");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG_MESSAGES
    else {
        fprintf(stderr, "vmem successfully created\n");
    }
#endif /* DEBUG_MESSAGES */

    /* Setup signal handler */
    /* Handler for USR1 */
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if(sigaction(SIGUSR1, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR1");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG_MESSAGES
    else {
        fprintf(stderr, "USR1 handler successfully installed\n");
    }
#endif /* DEBUG_MESSAGES */

    if(sigaction(SIGUSR2, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR2");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG_MESSAGES
    else {
        fprintf(stderr, "USR2 handler successfully installed\n");
    }
#endif /* DEBUG_MESSAGES */

    if(sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("Error installing signal handler for INT");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG_MESSAGES
    else {
        fprintf(stderr, "INT handler successfully installed\n");
    }
#endif /* DEBUG_MESSAGES */

    /* Signal processing loop */
    while(1) {
        signal_number = 0;
        pause();
        if(signal_number == SIGUSR1) {  /* Page fault */
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGUSR1\n");
#endif /* DEBUG_MESSAGES */
            signal_number = 0;
        }
        else if(signal_number == SIGUSR2) {     /* PT dump */
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGUSR2\n");
#endif /* DEBUG_MESSAGES */
            signal_number = 0;
        }
        else if(signal_number == SIGINT) {
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGINT\n");
#endif /* DEBUG_MESSAGES */
        }
    }


    return 0;
}

/* Please DO keep this function unmodified! */
void
logger(struct logevent le)
{
    fprintf(logfile, "Page fault %10d, Global count %10d:\n"
            "Removed: %10d, Allocated: %10d, Frame: %10d\n",
            le.pf_count, le.g_count,
            le.replaced_page, le.req_pageno, le.alloc_frame);
    fflush(logfile);
}









/*initialize pagefile*/
void init_pagefile(const char *pfname){
    
//    int content_for_pagefile[VMEM_VIRTMEMSIZE];  //size of array = 1024 -> space in pagefile
//    
//    srand(SEED); //Method of c library: void srand(unsigned int seed)
//    
//    int i;
//    for(i = 0 ; i < VMEM_VIRTMEMSIZE ; i++ ) {
//        content_for_pagefile[i] = rand() % 1000;  //Zahlen zwischen 0..99
//    }
//    
//    FILE *data_out; 
//    data_out = fopen( MMANAGE_PFNAME, "w");
//    
//    //fwrite(Array, Type in Array, Size of Array, Filename)
//    if(!(fwrite(&content_for_pagefile, sizeof(int), VMEM_VIRTMEMSIZE, data_out))){
//        perror("hulzkjhwhjtkl");
//        exit(EXIT_FAILURE);
//    }
    
    int itemNumbers = VMEM_NPAGES * VMEM_PAGESIZE;
	int data[itemNumbers];

	srand(SEED); // Zufallszahlen-Seed randomisieren

	int i;

  // Auslagerungsdatei mit zufälligen Werten...
	for(i = 0; i < itemNumbers; i++) {
		data[i] = rand() % 1000; // ...zwischen 0-999 füllen
	}

  // Dateistream öffnen...
	pagefile = fopen(pfname, "w+b");
	if(!pagefile) {
		perror("Error creating pagefile");
		exit(EXIT_FAILURE);
	}

  // ...und Daten ausschreiben
	int write_result = fwrite(data, sizeof(int), itemNumbers, pagefile);
	if(!write_result) {
		perror("Error creating pagefile");
		exit(EXIT_FAILURE);
	}

    
}








/* initialize virtual memory*/
void vmem_init(void){
    key_t shm_key = 23456;
    int shm_id = -1;
    
    /* Create shared memory 
     * ftok() generates an IPC key based on path and id */
//    shm_key = ftok(SHMKEY, SHMPROCID);
    
    /* Set the IPC_CREAT flag 
     * shmget(Shared Memory kann von mehr als ein Prozess zugegriffen werden, Größe des Shared Memory in Bytes, ein Flag)*/
    shm_id = shmget(shm_key, SHMSIZE, 0664 | IPC_CREAT);
    
    if (shm_id == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    /* Shared Memory für das Programm verfügbar machen */
    vmem = shmat(shm_id, NULL, 0);
                                                        //http://man7.org/linux/man-pages/man2/shmat.2.html
    if (vmem == (void *)-1) {
        fprintf(stderr, "making the shared memory accessible to the program (shmat) failed\n");
        exit(EXIT_FAILURE);
    }
    
    /* Pagetable initialisieren: */ 
    /* size of ? 128*8=1024 */
    vmem->adm.size = VMEM_VIRTMEMSIZE;   /* (VMEM_NPAGES * VMEM_PAGESIZE) = VMEM_VIRTMEMSIZE = ((VMEM_VIRTMEMSIZE / VMEM_PAGESIZE) * VMEM_PAGESIZE) */
    vmem->adm.mmanage_pid = getpid();
    vmem->adm.shm_id = shm_id;
    
    /* unterhalb der Prozesse teilen 
     *  int sem_init(sem_t *sem, int pshared, unsigned int value)
     *  initialisiert das namenslose Semaphore an der Adresse des 
     *  Pointers sem. Das Argument value spezifiziert den Wert des Semaphors.
     *  Ist pshared = 0 dann wird das Semaphore nur unter Threads eines
     *   Prozesses geteilt. 
     *  Ist pshared = 1 (bzw. eine Zahl != 0) dann wird das Semaphore unter
     *   mehreren Prozessen geteilt. */
    int semaphore = sem_init(&(vmem->adm.sema), 1, 0);
    if(semaphore == -1){
        fprintf(stderr, "Semaphore couldn\'t initialized\n");
        exit(EXIT_FAILURE);
    }
    
    return;
    
}







void sighandler(int signo){
    /* if the input is the same as SIGUSR1 then allocate a page*/
    if (signo == SIGUSR1){
        allocate_page();
    }else if(signo == SIGUSR2){
        dump_pt();
    }else if(signo == SIGINT){
        cleanup();
        exit(EXIT_SUCCESS);
    }
}







/* Seite zuweisen/belegen/reservieren */
void allocate_page(void){
    int requested_page_number = vmem->adm.req_pageno; /* Die Nummer der angeforderten Seite (befindet sich im Struct vmem_adm_struct)*/
    int free_space_in_bitmap = VOID_IDX;              /* in Hilfe heißt es frame*/
    int removed_page_id = VOID_IDX;
    
    /* wenn Page schon geladen */
    if(vmem->pt.entries[requested_page_number].flags & PTF_PRESENT){
        /* dann gehe aus der Funktion heraus */
        return;
    }
    
    
    /* freien Platz in Bitmap suchen
     * return -1, dann keinen freien Platz verfügbar
     * sonst gibt Platz zurück */
//    free_space_in_bitmap = search_bitmap();
    
    /* Einfacher ist es:
     * Am Anfang sind alle Plätze. Der Zähler index_counter wird hochgezählt
     * um den nächsten Index in dem man eine Page eintragen kann. Bis zur
     * maximalen Größe minus eins des physikalischen Speichers muss kein Pagefault ausgeführt 
     * werden, danach immer!*/
    if(index_counter < VMEM_NFRAMES){
        free_space_in_bitmap = index_counter;
        index_counter++;
    }
   
    
    
    if(free_space_in_bitmap != VOID_IDX){
        
        fprintf(stderr, "Found free frame no %d, allocaing page\n", free_space_in_bitmap);
        
    }else{   
        /* Wenn kein Platz verfügbar ist
           mache je nach Algorithmus einen Platz frei */
        free_space_in_bitmap = find_remove_frame();  
        
        /* schreibe den freien Platz in die Struktur */
        removed_page_id = vmem->pt.framepage[free_space_in_bitmap];
        
        Pagefault_no++;
        printf("Pagefault number: %d,   ", Pagefault_no);
        printf("free frame number %d \n", free_space_in_bitmap);
        
        /* pb_flag_of_page: Present-Bit der Seite */
//        int pb_flag_of_page = vmem->pt.entries[removed_page_id].flags;
        
    }
    
    /* Wenn Seite (Page) geändert wurde */
    if(vmem->pt.entries[removed_page_id].flags & PTF_DIRTY){  /* wenn das Flag des freigemachten Platzes auf 2 (Dirty) gesetzt ist */    /*--------------------- HIER EVTL. STATT == EIN & !!!*/
        store_page(removed_page_id);                           /* dann speichere die Seite in der Pagefile */
    }

    /* Das Flag wird zurückgesetzt, indem es bitweise mit dem Komplement
     * von dem Present-Bit addiert wird.
     * D.h. Flag 1, Present-Bit 1  
     *                 0001 (Flag ist 1)
     *                &0000 (Komplement von Present-Bit 1)
     *                =0000 (Flag ist jetzt 0)
     */
    vmem->pt.entries[removed_page_id].flags = (vmem->pt.entries[removed_page_id].flags & ~PTF_PRESENT);  
        
    
    
    /* Freie Stelle wird geupdated */
    update_pt(free_space_in_bitmap);
    fetch_page(requested_page_number);
    
    /* Update page fault counter */
    vmem->adm.pf_count++;
    
    /* Log action */
    struct logevent levcent;
    levcent.req_pageno = requested_page_number;
    levcent.replaced_page = removed_page_id;          
    levcent.alloc_frame = free_space_in_bitmap;
    levcent.pf_count = vmem->adm.pf_count;
    levcent.g_count = vmem->adm.g_count;
    logger(levcent);
    
    /* Unblock application */
    sem_post(&(vmem->adm.sema));
    
}








/* Speicherauszug/Ansicht von pagetable*/
void dump_pt(void){
    
    /* int fprintf(FILE *stream, const char *format, ...) 
     * stream -> Pointer auf die Datei
     * format -> Stringformatierung der Ausgabe */
    fprintf(stderr, "Dump pagetable\n\n");

    fprintf(stderr, "admin struct\n");
    fprintf(stderr, "------------\n");
    fprintf(stderr, "size: %d, pf_count: %d\n", vmem->adm.size, vmem->adm.pf_count);
    fprintf(stderr, "req_pageno: %d, next_alloc_idx: %d\n", vmem->adm.req_pageno, vmem->adm.next_alloc_idx);
    fprintf(stderr, "g_count: %d\n\n", vmem->adm.g_count);

    fprintf(stderr, "data\n");
    fprintf(stderr, "----\n");
    
    int i; 
    // Daten des Shared Memory ausgeben
    for(i = 0; i < (VMEM_NFRAMES * VMEM_PAGESIZE); i++) {
        fprintf(stderr, "idx %d: %d\n", i, vmem->data[i]);
    }
}






/* Aufräumen des Programms bei Beendigung:
 *  - Shared Memory freigeben
 *  - Semaphor löschen
 *  - Dateien schließen
*/
void cleanup(void){
    /* Shared Memory freigeben */
    int shm_id = vmem->adm.shm_id;
    /* Erlaubnis des Shared Memory ändern */
    shmctl(shm_id, IPC_RMID, 0);
    
    
     /* Semaphore löschen */
    if (fclose(logfile) != 0){
        perror("Error closing logfile");
        exit(EXIT_FAILURE);
    }
    
    /* datei Pagefile schließen */
    if (fclose(pagefile) != 0){
        perror("Error closing pagefile");
        exit(EXIT_FAILURE);
    }
       
}







/* Seite (Page) aus der Pagefile holen, 
 * um damit weiterarbeiten zu können 
 * Parameter: pt_idx: Seitennummer (virtueller Speicher)*/
void fetch_page(int pt_idx){
    
    /* sizeof(): Rückgabewert ist die Größe des Typs in Bytes
     * VMEM_PAGESIZE: hier 8 Items per page 
     * Beispiel: pt_idx = 2, sizeof(int) = 4Bit, VMEM_PAGESIZE = 8
     * 2*4*8 = 64 --> 0010 * 0100 * 1000 = 000001000000 */
    int offset = pt_idx * sizeof(int) * VMEM_PAGESIZE;
    
    /* in der Seitentabelle die integer Zahl im physikalischen Speichers, 
     * die zu der Zahl pt_idx aus dem virtuellen Speicher gehört 
     * pt.entries[pt_idx]: der Eintrag in der Seitentabelle an der Stelle von 
     * pt_idx im virtuellen Speicher von */
    int frame = vmem->pt.entries[pt_idx].frame;
    
    /* */
    int *pstart = &(vmem->data[frame * VMEM_PAGESIZE]);
    
    /* fseek: die Funktion ändert die Position der Datei des spezifizierten Flusses. 
     * SEEK_SET: beginning of file, 
     * SEEK_CUR: Current Position of the file pointer,
     * SEEK_END: End of file */
    if(fseek(pagefile, offset, SEEK_SET) == -1){
        perror("Positioning in pagefile failed! ");
        exit(EXIT_FAILURE);
    }
    
    fread(pstart, sizeof(int), VMEM_PAGESIZE, pagefile);
}







/* Die übergebene Pagetable_ID in die Pagefile speichern 
 * MMANAGE_PFNAME  = "./pagefile.bin" */
void store_page(int pt_idx){
    int offset = pt_idx * sizeof(int) * VMEM_PAGESIZE;
    int frame = vmem->pt.entries[pt_idx].frame;
    int *pstart = &vmem->data[frame * VMEM_PAGESIZE];
    
    /* Position in Pagefile setzten mithilfe des offset 
     * SEEK_SET entspricht der Anfangsposition */
    if (fseek(pagefile, offset, SEEK_SET) == VOID_IDX){
        perror("positioning in Pagefile failure");
        exit(EXIT_FAILURE);
    }
    
    /* write gibt die Anzahl der in die Pagefile geschriebenen Bytes zurück.
     * Bei einem Fehler wird -1 zurückgeliefert */
    if(!fwrite(pstart, sizeof(int), VMEM_PAGESIZE, pagefile)){
        perror("writing in Pagefile failure");
        exit(EXIT_FAILURE);
    }
    
    
}







/* Die Seitentabelle (Pagetable) aktualisieren. D.h. Änderungen speiern
 * Parameter: frame: Seitenrahmennummer */
void update_pt(int frame){
    int page_idx = vmem->adm.req_pageno;
    int bm_idx = frame / VMEM_BITS_PER_BMWORD;
    int bit = frame % VMEM_BITS_PER_BMWORD;
    
    /* Bitmap aktualisieren */
    vmem->adm.bitmap[bm_idx] |= (1U << bit);
    
    /* next_alloc_idx (nächster seitenindex) inkrementieren */
    vmem->adm.next_alloc_idx = (vmem->adm.next_alloc_idx + 1) % VMEM_NFRAMES;
    
    /* Seitenrahmen aktualisieren */
    vmem->pt.framepage[frame] = page_idx;
    
    /* pt_entry aktualisieren
     * flags: R-Bit (PTF_USED), M-Bit  (PTF_DIRTY), Present-Bit
     * frame
     * count*/
    vmem->pt.entries[page_idx].flags |= PTF_USED | PTF_PRESENT;
    vmem->pt.entries[page_idx].flags &= ~PTF_DIRTY;
    vmem->pt.entries[page_idx].frame = frame;
    vmem->pt.entries[page_idx].count = vmem->adm.g_count;
    
}







/* Findet einen Frame der freigemacht werden kann,
 * um etwas neues dort einlagern zu können. 
 * Die Findung hängt von den jeweiligen Algorithmen (FIFO, CLOCK, LRU) ab
 * return: freier Platz*/
int find_remove_frame(void){
    int remove_frame = VOID_IDX;
    
    /* Je nach der Belegung von VMEM_ALGO wird der zugehörige 
     * Algorithmus für die suche ein Frame austauschen zu können.
     * wird weder LRU noch CLOCK angegeben so erfolg die Suche mit
     * hilfe des FIFO-AlgSorithmus */
    switch (VMEM_ALGO) {
        case VMEM_ALGO_LRU:   remove_frame = find_remove_lru(); break;
        case VMEM_ALGO_CLOCK: remove_frame = find_remove_clock(); break;
        case VMEM_ALGO_FIFO:
        default:              remove_frame = find_remove_fifo(); break; 
    }
    
    return remove_frame;
}








/* Algorithmus FIFO */
int find_remove_fifo(void){
    /* index des Frames holen, welches vorne am Kopf steht 
     *  -> ältestes Einfügungsdatum */
    int remove_frame = vmem->adm.next_alloc_idx;
    return remove_frame;
}








/* Algorithmus LRU */
int find_remove_lru(void){
    int i;
    int remove_frame = VOID_IDX;
    /* Maximaler Wert (Java -> MAX_VALUE) */
    unsigned int smallest_count = -1;
  
    /* Frame suchen, welches das älteste Benutzungsdatum hat, welches
     * also lange nicht mehr angefasst wurde */ 
    for(i = 0; i < VMEM_NFRAMES; i++){
        /* Hole nächste Seite */
	int page = vmem->pt.framepage[i];
        
        /* wenn Seite kleiner (also Datum länger zurückliegt) als smallest_count,
         * dann setzte smallest_count auf das Datum dieser Seite. 
         * smallest_count ist am Anfang unendlich groß, sodass jeder Seite 
         * kleiner wäre */
        if(vmem->pt.entries[page].count < smallest_count){
            smallest_count = vmem->pt.entries[page].count;
            /* Nummer des Frames mit dem, bis hier hin, "kleinsten" 
             * Zeitstempel setzen */
            remove_frame = i;
	}
    }
    
    return remove_frame;
}








/* Algorithmus CLOCK*/
int find_remove_clock(void){
    int remove_frame = vmem->adm.next_alloc_idx;
    int frame = remove_frame;
    int page;
    
    /* Gehe die Schleife solange durch, bis eine Seite gefunden wurde, 
     * die ersetzt werden kann. Erkennlich daran, dass das Referenced-Bit (PTF_USED)
     * Null ist. Nicht ersetztbare Seiten haben das R-Bit=1, diese werden
     * auf Null geändert und es wird ein Frame weitergegangen. */
    while(1) {
        page = vmem->pt.framepage[frame];
        
        if(vmem->pt.entries[page].flags & PTF_USED) {
           vmem->pt.entries[page].flags &= ~PTF_USED;
           /* lösche References-Bit*/
           frame = (frame + 1) % VMEM_NFRAMES; 
        }else { 
           /* Frame kann ersetzt werden, da R-Bit==0 */
            remove_frame = frame; 
            break; 
        }
    }/* end while */
    
    vmem->adm.next_alloc_idx = remove_frame;
    
    return remove_frame;
}








/* Freien Platz (alles != -1) in der Bitmap suchen.
 * return: Position/Adresse des freien Bits 
 * Kontstanten und Struct in vmem.h */
int search_bitmap(void){
    int i;
    int free_bit = VOID_IDX;
    
    for(i = 0; i< VMEM_BMSIZE; i++){
        
        Bmword bitmap = vmem->adm.bitmap[i];
        Bmword mask = (i == (VMEM_BMSIZE - 1) ? VMEM_LASTBMMASK : 0);
        free_bit = find_free_bit(bitmap, mask);
        
        if(free_bit != VOID_IDX){
            int offset = i * VMEM_BITS_PER_BMWORD;
            free_bit = free_bit + offset;
            break;          /* wenn free_bit != -1 dann gehe aus der for-Schleife raus*/
        }
    }
    
    
    return free_bit;
}









/* Findet die Adresse des freien Platzes in der Bitmap 
 * -> Hilffunktion*/
int find_free_bit(Bmword bmword, Bmword mask){
    int bit = VOID_IDX;
    
    /* Bitmaske wird mit 1 initialisiert */
    Bmword bitmask = 1;
    
    bmword = (bmword | mask); /* Bitweise "oder" rechnen */
    
    for(bit = 0; bit < VMEM_BITS_PER_BMWORD; bit++){
        
        /* */
        if(!(bmword & bitmask)){
            break;
        }
        
        bitmask = bitmask << 1; /* shift nach links um 0001 */
    }
    
    return (bit < VMEM_BITS_PER_BMWORD) ? (bit) : (VOID_IDX);
}
