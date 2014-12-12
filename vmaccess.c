/*
 
 
 */

#include "vmaccess.h"

struct vmem_struct *vmem = NULL;

/* Connect to shared memory (key from vmem.h) */
void vm_init(void){
    
    key_t shm_key = 23456;
    int shm_id = -1;

    shm_id = shmget(shm_key, SHMSIZE, 0664 |IPC_CREAT);
    
    /* Shared Memory konnte nicht initialisiert werden */
    if(shm_id == -1){
      perror("Error initialising shmget");
      exit(EXIT_FAILURE);
    }

    vmem = shmat(shm_id, NULL, 0);
    
    if(vmem == (char *)-1){
      perror("Error initialising shmat");
      exit(EXIT_FAILURE);
   }
}





/* Physikalische Speicheradresse errechenen mit Hilfe der virtuellen Adresse */
int get_phys_index(int address, int permission){
    if(vmem == NULL){
        vm_init();
    }
    
    int page = address / VMEM_PAGESIZE;
  
    int offset = address % VMEM_PAGESIZE;
  
    /* Flags der page holen */
    int flags = vmem->pt.entries[page].flags;

    /* Ist Present-Bit gesetzt (XX1 & 1 == 1 || XX0 & 1 == 0) */
    int req_page_is_loaded = ((flags & PTF_PRESENT) == PTF_PRESENT);

    /* Page nicht geladen? */
    if(!req_page_is_loaded){
        
       
        
       /* Adresse der Page in mmanage speichern */
      vmem->adm.req_pageno = page;
      
      /* und 'mmanage' zum Laden der Page aufgefordert */
      kill(vmem->adm.mmanage_pid, SIGUSR1);
      
      /* vmaccess blockieren, um auf das Signal von 'mmanage',
       * zu warten */
      sem_wait(&vmem->adm.sema); // Signal wird über Semaphor erwartet
    }
    
    /* R-Bit (Used-Bit) setzen */
    vmem->pt.entries[page].flags |= PTF_USED;
    
    /* Prüfen, ob eine Schreiboperation durchgeführt wurde
     * dies wird nur durchgeführ, wenn vmem_write diese Funktion aufruft */
    if(permission){
      /* M-Bit (Dirty-Bit) setzen */
      vmem->pt.entries[page].flags |= PTF_DIRTY;
    }

    /* Inkrementiere den Global-Count g_count (Zugriff auf den Speichr ist erfolgt) 
     * -> relevant für LRU */
    vmem->adm.g_count++;
    
    /* Merke den Globalcount-Wert im Attribut 'count' für den aktuellen Frame 
     * -> relevant für LRU */
    vmem->pt.entries[page].count = vmem->adm.g_count;

    /* Physikalischen Speicher Index errechnen */
    return (vmem->pt.entries[page].frame * VMEM_PAGESIZE) + offset;
    
    
}





/* Von der "virtual" Adresse lesen */
int vmem_read(int address){
    
  //Physikalische Addresse holen
  int phys_addr = get_phys_index(address, 0);
  
  //Inhalt des Speichers zurückgeben
  return vmem->data[phys_addr];
}






/* Daten in die "virtual" Adresse schreiben */
void vmem_write(int address, int data){
    
   // Physikalische Adresse holen
  int phys_addr = get_phys_index(address, 1);
  
  // Inhalt des Speichers verändern
  vmem->data[phys_addr] = data;
  return;
}



