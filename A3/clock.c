#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int idx;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
  //if ref bit = 1, change it to 0
	while (coremap[idx].pte->frame & PG_REF){
		coremap[idx].pte->frame = coremap[idx].pte->frame & ~PG_REF;
		idx = (idx+1) % memsize;
	}
  //update the return index
  int ret = idx;
  //move global pointer to next
  idx = (idx+1) % memsize;
	return ret;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {

	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	idx = 0;
}
