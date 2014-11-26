/*
 
 
 */

#include "vmaccess.h"

/* Connect to shared memory (key from vmem.h) */
void vm_init(void){
    
}

/* Read from "virtual" address */
int vmem_read(int address);

/* Write data to "virtual" address */
void vmem_write(int address, int data);
