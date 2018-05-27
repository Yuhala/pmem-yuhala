/*
 * Copyright 2016, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * queue_array.c -- array based queue ; memory bounded
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <libpmemobj.h>

POBJ_LAYOUT_BEGIN(queue);
POBJ_LAYOUT_ROOT(queue, struct root);
POBJ_LAYOUT_TOID(queue, struct entry);
POBJ_LAYOUT_TOID(queue, struct queue);
POBJ_LAYOUT_END(queue);
void sig_handler(int sig);

struct entry { /* queue entry that contains arbitrary data */
	size_t len; /* length of the data buffer */
	char data[];
};

struct queue { /* array-based queue container */
	size_t front; /* position of the first entry */
	size_t back; /* position of the last entry */

	size_t capacity; /* size of the entries array, must be power of two */
	TOID(struct entry) entries[];
};

struct root {
	TOID(struct queue) queue;
	
};


//This flag controls the infinite loop
int flag=1;
//Flags to tell if item is in either queues
int inq1 =0, inq2=0;
void sig_handler(int sig){
	//Signal handler for ending while loop 
	printf("\nSignal handler launched\n");
	//if the signal received is ctrl+c || ctrl+z || kill 
	if(sig == SIGINT)
	{
		flag=0;	
	
		}
 
}

/*
 * queue_constructor -- constructor of the queue container
 */
static int
queue_constructor(PMEMobjpool *pop, void *ptr, void *arg)
{
	struct queue *q = ptr;
	size_t *capacity = arg;
	q->front = 0;
	q->back = 0;
	q->capacity = *capacity;

	/* atomic API requires that objects are persisted in the constructor */
	pmemobj_persist(pop, q, sizeof(*q));

	return 0;
}

/*
 * queue_new -- allocates a new queue container using the atomic API
 */
static int
queue_new(PMEMobjpool *pop, TOID(struct queue) *q, size_t nentries)
{
	return POBJ_ALLOC(pop,
		q,
		struct queue,
		sizeof(struct queue) + sizeof(TOID(struct entry)) * nentries,
		queue_constructor,
		&nentries);
}

/*
 * queue_nentries -- returns the number of entries
 */
static size_t
queue_nentries(struct queue *queue)
{
	return queue->back - queue->front;
}

/*
 * queue_enqueue -- allocates and inserts a new entry into the queue
 */
static int
queue_enqueue(PMEMobjpool *pop, struct queue *queue,
	const char *data, size_t len)
{
	if (queue->capacity - queue_nentries(queue) == 0)
		return -1; /* at capacity */

	/* back is never decreased, need to calculate the real position */
	size_t pos = queue->back % queue->capacity;

	int ret = 0;

	//printf("inserting %zu: %s\n", pos, data);

	TX_BEGIN(pop) {
		/* let's first reserve the space at the end of the queue */
		TX_ADD_DIRECT(&queue->back);
		queue->back += 1;

		/* and then snapshot the queue entry that we will allocate to */
		TX_ADD_DIRECT(&queue->entries[pos]);

		/* now we can safely allocate and initialize the new entry */
		queue->entries[pos] = TX_ALLOC(struct entry,
			sizeof(struct entry) + len);
		memcpy(D_RW(queue->entries[pos])->data, data, len);
	} TX_ONABORT { /* don't forget about error handling! ;) */
		ret = -1;
	} TX_END

	return ret;
}

/*
 * queue_dequeue - removes and frees the first element from the queue
 */
static int
queue_dequeue(PMEMobjpool *pop, struct queue *queue)
{
	if (queue_nentries(queue) == 0)
		return -1; /* no entries to remove */

	/* front is also never decreased */
	size_t pos = queue->front % queue->capacity;

	int ret = 0;

	//printf("removing %zu: %s\n", pos, D_RO(queue->entries[pos])->data);

	TX_BEGIN(pop) {
		/* move the queue forward */
		TX_ADD_DIRECT(&queue->front);
		queue->front += 1;
		/* and since this entry is now unreachable, free it */
		TX_FREE(queue->entries[pos]);
		/* notice that we do not change the PMEMoid itself */
	} TX_ONABORT {
		ret = -1;
	} TX_END

	return ret;
}



/*
 * fail -- helper function to exit the application in the event of an error
 */
static void __attribute__((noreturn)) /* this function terminates */
fail(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	


	if (argc < 3) {
		fail("usage: file-name item");
		return 0;
	}
	 
	//setting up the signal handler
	 if (signal(SIGINT, sig_handler) == SIG_ERR) printf("\nCan't catch SIGINT\n");

	//Initialising pool
	PMEMobjpool *pop = pmemobj_open(argv[1], POBJ_LAYOUT_NAME(queue));
	if (pop == NULL)
		fail("failed to open the pool");



	TOID(struct root) root1 = POBJ_ROOT(pop, struct root);
	TOID(struct root) root2 = POBJ_ROOT(pop, struct root);
	struct root *rootp1 = D_RW(root1);
	struct root *rootp2 = D_RW(root2);
	size_t capacity=1;


	//Creating both queues
	if (queue_new(pop, &rootp1->queue, capacity) != 0)
				fail("failed to create a new q1");
	if (queue_new(pop, &rootp2->queue, capacity) != 0)
				fail("failed to create a new q2");

	//Enqueuing item in q1
	TX_BEGIN(pop)
	{
			
	if (D_RW(rootp1->queue) == NULL)
				fail("q1 must exist");

	if (queue_enqueue(pop, D_RW(rootp1->queue),
				argv[2], strlen(argv[2]) + 1) != 0)
				fail("failed to insert item in q1");
	}
	TX_ONABORT
	{
		abort();
	}
	TX_END

	

	//While loop; enqueues and dequeues both queues
	while(flag){
		//Dequeue item from q1 and enqueue item in q2
		TX_BEGIN(pop) {
			inq1=0;inq2=1;
		if (D_RW(rootp1->queue) == NULL)
				fail("q1 must exist");

	if (queue_dequeue(pop, D_RW(rootp1->queue)) != 0)
				fail("failed to remove item from q1");
	
	if (D_RW(rootp2->queue) == NULL)
		
				fail("queue must exist");

	if (queue_enqueue(pop, D_RW(rootp2->queue),
				argv[2], strlen(argv[2]) + 1) != 0)
				fail("failed to insert item in q2");
		
	} TX_ONABORT {
		abort();
	} TX_END
	{
			printf("inserted item in q2\n");	
			printf("removed item from q1\n");	
	}

		//Dequeue item from q2 and enqueue item in q1
			TX_BEGIN(pop) {
			inq1=1;inq2=0;
		if (D_RW(rootp2->queue) == NULL)
				fail("q2 must exist");

	if (queue_dequeue(pop, D_RW(rootp2->queue)) != 0)
				fail("failed to remove item from q2");

	if (D_RW(rootp1->queue) == NULL)
				fail("q1 must exist");

	if (queue_enqueue(pop, D_RW(rootp1->queue),
				argv[2], strlen(argv[2]) + 1) != 0)
				fail("failed to insert item in q1");

	
				
	} TX_ONABORT {
		abort();
	} TX_END
		{
			printf("inserted item in q1\n");	
			printf("removed item from q2\n");	
		}
	}


		    if(inq1==1 && inq2==1){
				printf("\nItem in both queues\n");
				//printf("\nProgram incorrect");
				
			}

			if(inq1==0 && inq2==0){
				printf("\nItem present none of the queues");
				printf("\nProgram incorrect");
				
			}

		
			if((inq1==1 && inq2==0) || (inq1==0 && inq2==1)){
				printf("\nItem present in only one queue");
				printf("\nProgram correct\n");
				
			}	


	pmemobj_close(pop);

	return 0;
}


