/* Header file of sync_info_mem_store  data structure. We will implement given
 * structure as a hash table with dir_info values (we define that below).
 * We take source_dir as a key in the structure. The table allows duplicate values
 * is up to the user to manage them
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lnode.h"
#define INITIAL_SIZE 193 // initial size for our hash_table

#pragma once

typedef struct {
    // Our hash table uses seperate chaining
    lnode** hash_table; 
    int size;
    int capacity;
} sync_info_mem_store;

// Creates and initializes a sync_info_mem_store data structure
sync_info_mem_store* sync_info_mem_store_create(void);

// Adds new_value into structure
void sync_info_mem_store_add(sync_info_mem_store* mem,dir_info new_value);

// Removes value with given source_dir
void sync_info_mem_store_remove(sync_info_mem_store* mem,char* source_dir);

// Returns status of the source dir or NULL if it doesn't exist
dir_info* sync_info_mem_store_find(sync_info_mem_store* mem,char* source_dir);

// Frees the structure from the memory 
void sync_info_mem_store_destroy(sync_info_mem_store* mem);

