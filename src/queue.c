#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/queue.h"

// Creates a queue structure and returns it
queue* queue_create(void) {
     queue* pq = malloc(sizeof(queue));
     if (pq == NULL) {
         perror("ERROR! malloc failed\n");
         exit(-1);
     }
     // The queue doesn't have any values stored
     pq->head = NULL;
     pq->tail = NULL;
     return pq;
}

// Pushes a new action in the queue (FIFO Logic)
void queue_push_action(queue* pq,char* new_action) {
     qnode* qn = malloc(sizeof(qnode));
     if (qn == NULL) {
         perror("ERROR! malloc failed\n");
         exit(-1);
     }
     qn->action = malloc(sizeof(char) * (strlen(new_action) + 1));
     if (qn->action == NULL) {
         perror("ERROR! malloc failed\n");
         exit(-1);
     }
     strcpy(qn->action,new_action);
     qn->next = NULL;
     // The node is the first value in the structure
     if (pq->tail == NULL) {
         pq->tail = qn;
         pq->head = qn;
     }
     // We push node at the end, and make tail point at it
     else {
         pq->tail->next = qn;
         pq->tail = qn;
     }
}

// Returns true if the queue doesn't have any values stored
bool queue_is_empty(queue* pq) {
    return pq->head == NULL;
}

// Pops an action from the queue and returns it
char* queue_pop_action(queue* pq) {
    // We don't have any values stored
    if (pq->head == NULL)
        return "";
    char* ret = pq->head->action;
    qnode* temp = pq->head; // To free it later
    pq->head = temp->next;
    // Checking if we need to make the queue empty
    if (pq->head == NULL) {
        pq->tail = NULL;
    }
    free(temp);
    return ret;
}

// Frees the structure from memory
void queue_destroy(queue** pq) {
    while (!queue_is_empty(*pq)) {
        char* x = queue_pop_action(*pq);
        free(x);
    }
    free(pq);
}

