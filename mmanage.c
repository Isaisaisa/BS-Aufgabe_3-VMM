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
    
    
}

/* TODO weiterschrieben!!!!! */
void sighandler(int signo){
    /* if the input is the same as SIGUSR1 then allocate a page*/
    if (signo == SIGUSR1){
        allocate_page();
    }else if(signo == SIGUSR2){
        
    }
}


/* Seite zuweisen/belegen/reservieren */
void allocate_page(void){
    int required_page_number = vmem->adm.req_pageno;
    
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
 * um damit weiterarbeiten zu können*/
void fetch_page(int pt_idx){
    
}

/* Die übergebene Pagetable_ID in die Pagefile speichern 
 * MMANAGE_PFNAME  = "./pagefile.bin" */
void store_page(int pt_idx){
    
}

/* Die Seitentabelle (Pagetable) aktualisieren. D.h. Änderungen speiern*/
void update_pt(int frame){
    
}

/* Findet einen Frame der freigemacht werden kann,
 * um etwas neues dort einlagern zu können. 
 * Die Findung hängt von den jeweiligen Algorithmen (FIFO, CLOCK, LRU) ab*/
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

/* Freien Platz (0) in der Bitmap suchen.
 * Kontstanten und Struct in cmem.h */
int search_bitmap(void){
    return 0;
}


/* Findet die Adresse des freien Platzes in der Bitmap 
 * -> Hilffunktion*/
int find_free_bit(Bmword bmword, Bmword mask){
    return 0;
}
