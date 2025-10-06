/* Header file for a queue FIFO data structure. The fss manager will use this queue
 * in order to save the given actions, when the worker limit for worker
 * processes is reached */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#pragma once

// Queue node is a linked-list node that will be used inside queue
typedef struct queue_node qnode;

struct queue_node {
    char* action;
    qnode* next;
};

// Our queue data structure
typedef struct queue queue;

struct queue {
    qnode* head;
    qnode* tail;
};

// Creates a queue structure and returns it
queue* queue_create(void);

bool queue_is_empty(queue* pq);

// Pushes a new action in the queue (FIFO)
void queue_push_action(queue* pq,char* new_action);

// Pops an action from the queue and returns it (FIFO)
char* queue_pop_action(queue* pq);

// Frees the structure from memory
void queue_destroy(queue** pq);

