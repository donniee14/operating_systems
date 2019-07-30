/*	Donald Elmore
	File Description:
		This file contains all of the required functions for:
		malloc, free, realloc, and calloc; 
		along with a helper rounding function.

		Used a replacement for the above functions, link to replace.
	Known bugs:
		Not freeing correctly.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>

#define PAGESIZE 4096

// 0:2, 1:4, 2:8, 3:16, 4:32, 5:64, 6:128, 7:256, 8:512, 9: 1024, 10:larger
#define SIZES 11

typedef struct pageHeader {
	void *freeHead;
	void *freeHeadNext;
	void *freeTail;
	short int openBlks;
	struct pageHeader *nextPage;
} pageHead;

int fd = -1;
pageHead *listPtrs[SIZES] = {NULL};

/*
	Rounds x up to the next power of 2
	Pre: x is an int
	Post: x has been rounded up to the next power of 2
*/
int roundto2 (int x) {
	int result;
	if (x == 1) {
		result = 2;
	} else {
		result = pow(2, ceil(log(x)/log(2)));
	}
	return result;
}

/*
	Allocates size bytes and returns a pointer to the allocated memory
	Pre: size is a postive integer
	Post: a pointer to the malloced space is returned to the user
*/
void *malloc(size_t size) {
	void *retPtr = NULL;
	if (size < 1024)
		size = roundto2(size);
	if (fd == -1)
			fd = open("/dev/zero", O_RDWR);

	// determine what list the alloc will be pulled from
	int i;
	for (i = 1; i < 11; i++) {
		if (size == pow(2,i))
			break;
	} i--;

	// allocs for large objects
	if (size > 1024) {
		if (listPtrs[i] == NULL) {
			listPtrs[i] = mmap(NULL, size+sizeof(struct pageHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
			listPtrs[i]->openBlks = 0;
			listPtrs[i]->freeHead = (char *)listPtrs[i] + sizeof(struct pageHeader);
			listPtrs[i]->freeTail = (char *)listPtrs[i] + sizeof(struct pageHeader);
			listPtrs[i]->freeHeadNext = NULL;
			listPtrs[i]->nextPage = NULL;
			retPtr = listPtrs[i]->freeHead;
			return retPtr;
		}

		pageHead *temp = listPtrs[i];
		while (temp->nextPage != NULL) {
			temp = temp->nextPage;
		}

		temp->nextPage = mmap(NULL, size+sizeof(struct pageHeader), PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		temp->nextPage->openBlks = 0;
		temp->nextPage->freeHead = (char *)temp->nextPage + sizeof(struct pageHeader);
		temp->nextPage->freeTail = (char *)temp->nextPage + sizeof(struct pageHeader);
		temp->nextPage->freeHeadNext = NULL;
		temp->nextPage->nextPage = NULL;
		retPtr = temp->nextPage->freeHead;
		return retPtr;
	}

	// if first alloc of that size
	if (listPtrs[i] == NULL) {
		// set up free list
		listPtrs[i] = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
		listPtrs[i]->openBlks = (PAGESIZE - sizeof(struct pageHeader))/pow(2,i+1);
		listPtrs[i]->freeHead = (char *)listPtrs[i] + sizeof(struct pageHeader);
		listPtrs[i]->freeTail = (char *)listPtrs[i] + (PAGESIZE - size);
		listPtrs[i]->freeHeadNext = (char *)listPtrs[i]->freeHead + size;

		// remove from head of free list
		listPtrs[i]->openBlks--;
		retPtr = listPtrs[i]->freeHead;
		listPtrs[i]->freeHead = listPtrs[i]->freeHeadNext;
		listPtrs[i]->freeHeadNext = listPtrs[i]->freeHead + size;
		return retPtr;
	}

	// find the first page with space, if any
	int pageFound = 1;
	pageHead *temp = listPtrs[i];
	while (temp->openBlks < 1) {
		// if no empty page is found, temp will be last page in list
		if (temp->nextPage == NULL) {
			pageFound = 0;
			break;
		}
		temp = temp->nextPage;
	}

	// remove from head of found page
	if (pageFound == 1) {
		temp->openBlks--;
		retPtr = temp->freeHead;
		temp->freeHead = temp->freeHeadNext;
		temp->freeHeadNext = temp->freeHead + size;
		return retPtr;
	}

	// no page with space found, mmap new page, and linked to previous page
	else if (pageFound == 0) {
		// set up free list
		temp->nextPage = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE,
						MAP_PRIVATE, fd, 0);
		temp->nextPage->openBlks = (PAGESIZE-sizeof(struct pageHeader))/pow(2,i+1);
		temp->nextPage->freeHead = (char *)temp->nextPage + sizeof(struct pageHeader);
		temp->nextPage->freeTail = (char *)temp->nextPage + (PAGESIZE - size);
		temp->nextPage->freeHeadNext = (char *)temp->nextPage->freeHead + size;

		// remove from head of free list
		temp->nextPage->openBlks--;
		retPtr = temp->nextPage->freeHead;
		temp->nextPage->freeHead = temp->nextPage->freeHeadNext;
		temp->nextPage->freeHeadNext = temp->nextPage->freeHead + size;
		return retPtr;
	}
	return retPtr;
}

/*
	Frees the memory space pointed to by ptr
	Pre: ptr is a valid pointer to malloced memory
	Post: memory at ptr is freed
*/
void free(void *ptr) {
	return ptr;
}

/*
	Allocates numItems of size bytes and returns a pointer to the allocated memory
	Pre: size and numItems are postive integers
	Post: a pointer to the malloced space is returned to the user
*/
void *calloc(size_t numItems, size_t size) {
	if (numItems == 0 || size == 0)
		return NULL;
	if (size < 1024)
		size = roundto2(size);
	void *ptr = malloc(size * numItems);
	memset(ptr, 0, numItems * size);
	return ptr;
}

/*
	Changes the size of the memory block pointed to by ptr to size bytes
	Pre: ptr pointed to valid memory region, size is a positive integer
	Post: a pointer to the malloced space is returned to the user
*/
void *realloc(void *ptr, size_t size) {
	if (size == 0 && ptr != NULL)
		free(ptr);

	if (size < 1024)
		size = roundto2(size);
	void *new = malloc(size);

	if (ptr == NULL)
			return new;

	memcpy(new, ptr, size);
	return new;
}
