#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list_node.h"
#define INITIAL_SIZE 193 // initial size for our hash_table
                         //
#pragma once

typedef struct { 
    list_node** table;
    int size;
    int capacity;
} hash_table;


// Creates a hash table dynamically and returns a pointer to it
hash_table* hash_table_create(void);

// Inserts a pair into the hash table
void hash_table_insert(hash_table* map,int key,char* value);

// Returns the pair in the hash table that has the given key, or NULL if it 
// doesn't exist
pair* hash_table_find(hash_table* map,int key);

// Removes the pair with key from the map, if it exists
void hash_table_remove(hash_table* map,int key);
 
// Removes all values from map (doesn't free map itself)
void hash_table_destroy(hash_table* map);

// This function will be used for rehashing our table
void hash_table_rehash(hash_table* map);

// Finds the key with given value or -1. Not as optimal (O(N) complexity)
int hash_table_find_key(hash_table* map,char* value);
