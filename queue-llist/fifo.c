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
 * queue_llist.c - queue backed by linked list
 */

#include "pmemobj_list.h"
#include <ex_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include<unistd.h>


POBJ_LAYOUT_BEGIN(list);
POBJ_LAYOUT_ROOT(list, struct fifo_root);
POBJ_LAYOUT_TOID(list, struct tqnode);
POBJ_LAYOUT_END(list);

POBJ_TAILQ_HEAD(tqueuehead, struct tqnode);
void sig_handler(int sig);

//Persistent pool object
PMEMobjpool *pop;
//This flag controls the infinite loop
int flag = 1;
//Flags to tell if item is in either queues
int inq1 = 0, inq2 = 0;

struct fifo_root {
    struct tqueuehead head;
};

struct tqnode {
    char data;
    POBJ_TAILQ_ENTRY(struct tqnode) tnd;
};

static void
print_help(void) {
    printf("usage: program <poolfile> <item>");
}

void sig_handler(int sig) {
    //Signal handler for ending while loop 
    printf("\nSignal handler launched\n");
    //if the signal received is ctrl+c || ctrl+z || kill 
    if (sig == SIGINT) {
        flag = 0;

    }

}

int
main(int argc, const char *argv[]) {
    //path to poolfile
    const char *path;

    if (argc < 3) {
        print_help();
        return 0;
    }
    path = argv[1];

    //setting up the signal handler
    if (signal(SIGINT, sig_handler) == SIG_ERR) printf("\nCan't catch SIGINT\n");



    //Initialising pool
    if (file_exists(path) != 0) {
        if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(list),
                PMEMOBJ_MIN_POOL, 0666)) == NULL) {
            perror("failed to create pool\n");
            return -1;
        }
    } else {

        if ((pop = pmemobj_open(path, POBJ_LAYOUT_NAME(list))) ==
                NULL) {
            perror("failed to open pool\n");
            return -1;
        }
    }



    TOID(struct fifo_root) root = POBJ_ROOT(pop, struct fifo_root);
    //TOID(struct fifo_root) root2 = POBJ_ROOT(pop, struct fifo_root);

    //retrieving the two queues inside the pool
    struct tqueuehead *tqhead1 = &D_RW(root)->head;
    struct tqueuehead *tqhead2 = &D_RW(root)->head;

    TOID(struct tqnode) node;


    //Inserting item into q1 for the first time

    TX_BEGIN(pop) {
        inq1 = 1;
        node = TX_NEW(struct tqnode);
        D_RW(node)->data = *argv[2];
        POBJ_TAILQ_INSERT_HEAD(tqhead1, node, tnd);
    }
    TX_ONABORT
            {
        abort();}
    TX_END



    while (flag) {

        //Transaction to dequeue item from q1 and enqueue in q2

        TX_BEGIN(pop) {

            node = POBJ_TAILQ_LAST(tqhead1);
            //Enqueue item in q2
            POBJ_TAILQ_INSERT_HEAD(tqhead2, node, tnd);
            //Dequeue item from q1
            POBJ_TAILQ_REMOVE(tqhead1, node, tnd);
            inq1 = 0;
            inq2 = 1;

        }
        TX_ONABORT
                {
            abort();
}
        TX_END{
            printf("Dequeued item from q1 \n");
            printf("Enqueued item in q2 \n");
}

        TX_BEGIN(pop) {

            node = POBJ_TAILQ_LAST(tqhead2);
            //Enqueue item in q1
            POBJ_TAILQ_INSERT_HEAD(tqhead1, node, tnd);
            //Dequeue item from q2
            POBJ_TAILQ_REMOVE(tqhead2, node, tnd);
            inq1 = 1;
            inq2 = 0;
            //TOID(struct tqnode) node = TX_NEW(struct tqnode);	

        }
        TX_ONABORT
                {
            abort();}
        TX_END{
            printf("Dequeued item from q2 \n");
            printf("Enqueued item in q1  \n");
}



    }


    if (inq1 == 1 && inq2 == 1) {
        printf("\nItem in both queues\n");
        //printf("\nProgram incorrect");

    }

    if (inq1 == 0 && inq2 == 0) {
        printf("\nItem present in none of the queues\n");
        printf("\nProgram incorrect\n");

    }


    if ((inq1 == 1 && inq2 == 0) || (inq1 == 0 && inq2 == 1)) {
        printf("Item present in only one queue\n");
        printf("Program correct\n");

    }


    pmemobj_close(pop);
    return 0;
}

