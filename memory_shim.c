/*	Donald Elmore
	File Description:
		This file handles adding to and removing from the linked list along
			with reporting if there were any leaks, the size of the leaks, 
			and the total stats. 

		The malloc function is responsible for calling the 
			standard c malloc and adding nodes to the list. The free function
			is responsible for calling the standard c free, and removing nodes
			from the list.

		The destructor is where the stats are totaled and printed.

	Known bugs:
		None known.
*/

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

void *(*original_malloc)(size_t size) = NULL;
void (*original_free)(void *ptr) = NULL;

typedef struct list_node {
	int size;
	void *memory;
	struct list_node *next;
} node;

//will point to the start of the linked list
node *start = NULL;

void __attribute__ ((constructor)) construct(void) {
	original_malloc = dlsym(RTLD_NEXT, "malloc");
	original_free = dlsym(RTLD_NEXT, "free");
}

void __attribute__ ((destructor)) destruct(void) {
	int totLeakSize = 0;
	int remNodes = 0;
	node *iterator = NULL;
	iterator = start;

	while (iterator != NULL) {
		remNodes++;
		totLeakSize = totLeakSize + iterator->size;
		fprintf(stderr, "LEAK\t%d\n", iterator->size);
		iterator = iterator->next;
	}
	fprintf(stderr, "TOTAL\t%d\t%d\n", remNodes, totLeakSize);
}

void *malloc(size_t size) {
	void *origMem = original_malloc(size);
	node *mallocNode = original_malloc(sizeof(node));
	mallocNode->memory = origMem;
	mallocNode->size = size;
	mallocNode->next = NULL;
	
	if (start == NULL) {
		start = mallocNode;
		start->next = NULL;
	} else {
		node *temp = start;
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = mallocNode;
	}
	return origMem;
}

void free(void *ptr){
	original_free(ptr);

	node *previous = NULL;
    node *iterator = start;

    while (iterator != NULL) {
    	if (iterator->memory == ptr) {
    		if (iterator == start) {
    			start = iterator->next;
    		} else {
    			previous->next = iterator->next;
    		}
    		original_free(iterator);
    		return;
    	}
    	previous = iterator;
		iterator = iterator->next;
	}
}
