/*	Donald Elmore
	File Description:
		This file contains all of the required functions to implement
		an entire suite of thread managing functions.
	Known bugs:
		Condition variables are not correct.
*/

#include "mythreads.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

typedef struct threadDef {
	int id;						//thread id
	bool isDone;
	void *argsPtr;
	void *retVal;
	ucontext_t *context;		//changed to pointer
	thFuncPtr funPtr;			//function pointer
	struct threadDef *next;		//link to next thread
} thread;

thread *head;
thread *rover;

int totThreads;
int interruptsAreDisabled = 0;
int lockArray[NUM_LOCKS] = {0};
int condArray[NUM_LOCKS][CONDITIONS_PER_LOCK] = {0};

static void interruptDisable () {
	assert(!interruptsAreDisabled);
	interruptsAreDisabled = 1;
}

static void interruptEnable () {
	assert(interruptsAreDisabled);
	interruptsAreDisabled = 0;
}

/*
	Executes the thread function and marks the thread as done once finished
	Pre: the thread struct passed to the function is not NULL
	Post: isDone bit is set to true, and the thread function is complete
*/
void execute (thread *thrd) {
	interruptEnable();

	//assure that rover is in correct place
	while (rover->id != thrd->id) {
		rover = rover->next;
	}
	thrd->retVal = thrd->funPtr(thrd->argsPtr);
	while (rover->id != thrd->id) {
		rover = rover->next;
	}

	thrd->isDone = true;
	threadYield();
	interruptDisable();
}

/*
	Inits the thread library
	Pre: None
	Post: a new context and thread are malloced for the main thread
		-the main thread's struct values are set up
		-the circular list is initialized
*/
extern void threadInit() {
	interruptDisable();
	totThreads = 0;

	//create new context
	ucontext_t *newcontext = malloc(sizeof(ucontext_t));
	getcontext(newcontext);

	thread *headThread = malloc(sizeof(thread));
	headThread->context = newcontext;
	headThread->id = totThreads;
	headThread->isDone = false;
	headThread->funPtr = NULL;

	//make list circular
	headThread->next = headThread;

	head = headThread;
	rover = headThread;
	interruptEnable();
}

/*
	Creates a new thread
	Pre: funcPtr is not NULL, a function must exist to assign to the thread
	Post: a new context and thread are malloced and the thread is added to list
*/
extern int threadCreate(thFuncPtr funcPtr, void *argPtr) {
	//malloc a new context/thread to add to list
	ucontext_t *newcontext = (ucontext_t *)malloc(sizeof(ucontext_t));
	getcontext(newcontext);
	thread *newThread = malloc(sizeof(thread));
	newThread->context = newcontext;

	//set up context struct
	newThread->context->uc_stack.ss_sp = malloc(STACK_SIZE);
	newThread->context->uc_stack.ss_size = STACK_SIZE;
	newThread->context->uc_stack.ss_flags = 0;
	newThread->funPtr = funcPtr;
	newThread->argsPtr = argPtr;
	newThread->id = ++totThreads;	//increment totThreads and set new id

	makecontext(newThread->context, (void (*)(void)) execute, 1, newThread);

	//insert to list after rover
	thread *temp = rover->next;
	rover->next = newThread;
	newThread->next = temp;

    threadYield();
	return newThread->id;
}

/*
	Causes the currently running thread to yield to the next thread
	Pre: None
	Post: the context is swapped from the currently running thread to the
		next available thread
*/
extern void threadYield() {
	interruptDisable();
	thread *running = rover;

	//loop through list until running thread is found (not done)
	do {
		rover = rover->next;
	} while (rover->isDone == true && running->id != rover->id);

	//this prevents looping around list and swapping itself (one thread running)
	if (running->id != rover->id) {
		//swap currently running with not finish process
		swapcontext(running->context, rover->context);
	}
	interruptEnable();
}

/*
	Waits until the thread corresponding to id exits
	Pre: thread_id must exist within the list and result must
		be a valid point to a return value location
	Post: result is populated with the return of the thread function
*/
extern void threadJoin(int thread_id, void **result) {
	thread *temp = rover;
	while (temp->id != thread_id) {
		temp = temp->next;
	}
	while (temp->isDone != true) {
		threadYield();
	}

	*result = temp->retVal;
	//free(temp->context->uc_stack.ss_sp);
	//free(temp->context);
	//free(temp);
}

/*
	Exits the current thread, closing the main thread will term. the prog.
	Pre: result is a valid pointer than data can be set to
	Post: the return value of the function is set to result
		-the thread isDone bit is set
		-the thread's stack is freed
*/
extern void threadExit(void *result) {
	interruptDisable();
	rover->retVal = result;		//save result and set to be finished
	rover->isDone = true;
	free(rover->context->uc_stack.ss_sp);
	interruptEnable();
    threadYield();
}

/*
	Blocks until it is able to acquire the specificed lock
	Pre: lockNum < NUM_LOCKS
	Post: the element lockNum is set to 1 in lockArray
*/
extern void threadLock(int lockNum) {
	while (lockArray[lockNum] == 1) {
		threadYield();
	}
	interruptDisable();
	lockArray[lockNum] = 1;
	interruptEnable();
} 

/*
	Unlocks the specified lock
	Pre: lockNum < NUM_LOCKS
	Post: element lockNum of lockArray is set to 0
*/
extern void threadUnlock(int lockNum) {
	lockArray[lockNum] = 0;		//unlock
} 

/*
	This function unlocks the specified mutex lock and causes the
	current thread to block until the specified condition is signaled
	Pre: lockNum < NUM_LOCKS && conditionNum < CONDITIONS_PER_LOCK
	Post: the condition variable specified by lockNum and conditionNum
		is the waiting condition for the currently running thread
*/
extern void threadWait(int lockNum, int conditionNum) {
	interruptDisable();
	if (lockArray[lockNum] == 0) {
		printf("\nERROR: threadWait was called on an unlocked mutex!\n");
		exit(1);
	}
	threadUnlock(lockNum);
	while (condArray[lockNum][conditionNum] == 0) {
		threadYield();
	}

    threadLock(lockNum);
	interruptEnable();
}

/*
	Unlock a single thread waiting on the spec. condition var.
	Pre: lockNum < NUM_LOCKS && conditionNum < CONDITIONS_PER_LOCK
	Post: condArray[lockNum][conditionNum]'s value is set to 1
*/
extern void threadSignal(int lockNum, int conditionNum) {
	interruptDisable();
	condArray[lockNum][conditionNum] = 1;
	interruptEnable();
}
