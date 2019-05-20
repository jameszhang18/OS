#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#define MAXLINE 256


extern int memsize;

extern struct frame *coremap;

extern char *tracefile;

// create a struct node to obtain its vaddr and the next node 
typedef struct node
{
    struct node *next;
    addr_t vaddr;  
} node;

node* curr;
node* head;
node* tail;

/* Fing the frame distance between the current node and the next closest 
 * node which has the same vaddr, return -1 if not founded.
*/
int get_frame_distance(struct frame f) {
	int distance = 0;
	node* temp = head->next;
	while (temp) {
		if (temp->vaddr == f.vaddr)
			return distance;
		distance++;
		temp = temp->next;
	}
	
	return -1;
}


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int idx = 0;
	int frame = -1;
	int max_distance = -1;
	// Find the frame which has longest distance with the next frame who 
	// share the same vaddr
	while (idx < memsize){
		// Get the distance of current frame
		int distance = get_frame_distance(coremap[idx]);

		// If the frame not in the list, return it
		if (distance == -1)
			return idx;
		else if (distance > max_distance) {
			frame = idx;
			max_distance = distance;
		}
		idx++;
	}
	
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {

	// Free the current node and move to the next node.
	if (head != tail) {
		node* temp = head;
		head = head->next;
		free(temp);
	} 
	// If the current node is the last one, just free it, the curr and the 
	// tail will be freed too.
	else{
		free(head);
  }
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {

	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	FILE *tfp;

	// Read the tracefile first
	if((tfp = fopen(tracefile, "r")) == NULL) {
		perror("Error: opening tracefile");
		exit(1);
	}

	// Initialize the linked list
	while(fgets(buf, MAXLINE, tfp) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			
			node *new_node = (node *) malloc(sizeof(node));
			new_node->vaddr = vaddr;
			new_node->next = NULL;
			// If has head, continue to add node
			if (head)
				curr->next = new_node;
			// If there is no head, make the new node head
			else
				head = new_node;
			curr = new_node;
		} else {
			continue;
		}
		tail = curr;
	}
}

