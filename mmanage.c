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
    
    int content_for_pagefile[VMEM_VIRTMEMSIZE];  //size of array = 1024 -> space in pagefile
    
    srand(SEED); //Method of c library: void srand(unsigned int seed)
    
    int i;
    for(i = 0 ; i < VMEM_VIRTMEMSIZE ; i++ ) {
        content_for_pagefile[i] = rand() % 100;  //Zahlen zwischen 0..99
    }
    
    FILE *data_out; 
    data_out = fopen( MMANAGE_PFNAME, "w");
    
    //fwrite(Array, Type in Array, Size of Array, Filename)
    fwrite(&content_for_pagefile, sizeof(int), VMEM_VIRTMEMSIZE, data_out);
    
}








/* initialize virtual memory*/
void vmem_init(void){
    key_t shm_key = 0;
    int shm_id = -1;
    
    /* Create shared memory 
     * ftok() generates an IPC key based on path and id */
    shm_key = ftok(SHMKEY, SHMPROCID);
    
    /* Set the IPC_CREAT flag 
     * shmget(Shared Memory kann von mehr als ein Prozess zugegriffen werden, Größe des Shared Memory in Bytes, ein Flag)*/
    shm_id = shmget(shm_key, SHMSIZE, 0664 | IPC_CREAT);
    
    if (shm_id == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    
    /* Shared Memory für das Programm verfügbar machen */
    int vmem = shmat(shm_id, NULL, 0);
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







/* ------------------------------TODO weiterschreiben!!!!! ----------------------*/
void sighandler(int signo){
    /* if the input is the same as SIGUSR1 then allocate a page*/
    if (signo == SIGUSR1){
        allocate_page();
    }else if(signo == SIGUSR2){
        
    }
}







/* Seite zuweisen/belegen/reservieren */
void allocate_page(void){
    int requested_page_number = vmem->adm.req_pageno; /* Die Nummer der angeforderten Seite (befindet sich im Struct vmem_adm_struct)*/
    int free_space_in_bitmap = VOID_IDX;              /* ist Hilfe heißt es frame*/
    int removed_page_id = VOID_IDX;
    
    /* wenn Page schon geladen */
    if(vmem->pt.entries[req_pageno].flags & PTF_PRESENT){
        /* dann gehe aus der Funktion heraus */
        return;
    }
    
    
    /* freien Platz in Bitmap suchen
     * return -1, dann keinen freien Platz verfügbar
     * sonst gibt Platz zurück */
    free_space_in_bitmap = search_bitmap();
    
    
    if(free_space_in_bitmap != VOID_IDX){
        
        fprintf(stderr, "Found free frame no %d, allocaing page\n", free_space_in_bitmap);
        
    }else{   
        /* Wenn kein Platz verfügbar ist
           mache je nach Algorithmus einen Platz frei */
        free_space_in_bitmap = find_remove_frame();  
        
        /* schreibe den freien Platz in die Struktur */
        removed_page_id = vmem->pt.framepage[free_space_in_bitmap];
        
        /* pb_flag_of_page: Present-Bit der Seite */
        int pb_flag_of_page = vmem->pt.entries[removed_page_id].flags;
        
    }
    
    /* Wenn Seite (Page) geändert wurde */
    if(pb_flag_of_page == PTF_DIRTY){  /* wenn das Flag des freigemachten Platzes auf 2 (Dirty) gesetzt ist */    /*--------------------- HIER EVTL. STATT == EIN & !!!*/
        store_page(removed_page_id);                           /* dann speichere die Seite in der Pagefile */
    }

    /* Das Flag wird zurückgesetzt, indem es bitweise mit dem Komplement
     * von dem Present-Bit addiert wird.
     * D.h. Flag 1, Present-Bit 1  
     *                 0001 (Flag ist 1)
     *                &0000 (Komplement von Present-Bit 1)
     *                =0000 (Flag ist jetzt 0)
     */
    pb_flag_of_page = (pb_flag_of_page & ~PTF_PRESENT);  
        
    
    
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
    
}






/* Aufräumen des Programms bei Beendigung:
 *  - Shared Memory freigeben
 *  - Semaphor löschen
 *  - Dateien schließen
*/
void cleanup(void){
    
}







/* Seite (Page) aus der Pagefile holen, 
 * um damit weiterarbeiten zu können 
 * Parameter: pt_idx: Seitennummer (virtueller Speicher)*/
void fetch_page(int pt_idx){
    
}







/* Die übergebene Pagetable_ID in die Pagefile speichern 
 * MMANAGE_PFNAME  = "./pagefile.bin" */
void store_page(int pt_idx){
    
}







/* Die Seitentabelle (Pagetable) aktualisieren. D.h. Änderungen speiern
 * Parameter: frame: Seitenrahmennummer */
void update_pt(int frame){
    
}







/* Findet einen Frame der freigemacht werden kann,
 * um etwas neues dort einlagern zu können. 
 * Die Findung hängt von den jeweiligen Algorithmen (FIFO, CLOCK, LRU) ab
 * return: freier Platz*/
int find_remove_frame(void){
    return 0;
}








/* Algorithmus FIFO */
int find_remove_fifo(void){
    return 0;
}








/* Algorithmus LRU */
int find_remove_lru(void){
    return 0;
}








/* Algorithmus CLOCK*/
int find_remove_clock(void){
    return 0;
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
