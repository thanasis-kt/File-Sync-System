#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INITIAL_CAPACITY 193 // initial size for our hash_table
                         //
#pragma once

typedef struct  {
    int key;
    char* value;
} pair;

typedef struct lst list_node;

struct lst {
    pair value;
    list_node* next;
};

// Creates a list node dynamically and returns a pointer to it
list_node* list_node_create(int key,char* value,list_node* next);

// Adds new_value to the list
void list_node_add(list_node* lst,int key,char* new_value);

// Removes value with given key from the key
void list_node_remove(list_node** lst,int key);

// Returns the pair in the list_node that has the given key, or NULL if it 
// doesn't exist
pair* list_node_find(list_node* lst,int key);

// Frees list_node from memory (and it's values)
void list_node_destroy(list_node** node);
