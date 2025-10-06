/* sync_info_mem_store is a hash_table implemented using seperate chaining. It 
 * stores dir_info values and uses target_dir as primary key.
 * */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/sync_info_mem_store.h"
#include "../include/lnode.h"

#define LOAD_FACTOR 0.9 // when  size / capacity > load factor, we rehash

// According to data structure theory, we improve our hash table efficiency by
// selecting a prime integer as array's size. Also to accieve a O(1) complexity in
// rehashing we use at least double the size. So this array helps to accieve 
// those goals
int PRIME_SIZES[] = {53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241,
	786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};


// This function returns a hash code for the string value
uint hash_code(char* value);

// This function will be used to rehash our hash_table for efficiently
void sync_info_mem_store_rehash(sync_info_mem_store* mem);

// Creates and initializes a sync_info_mem_store data structure
sync_info_mem_store* sync_info_mem_store_create(void) {
    sync_info_mem_store* mem = malloc(sizeof(sync_info_mem_store));
    if (mem == NULL) {
        perror("ERRROR! malloc failed\n");
        exit(-1);
    }
    mem->size = 0;
    mem->capacity = INITIAL_SIZE;
    mem->hash_table = malloc(sizeof(lnode*) * mem->capacity);
    if (mem->hash_table == NULL) {
        perror("ERRROR! malloc failed\n");
        exit(-1);
    }
    // We initialize with NULL values
    for (int i = 0; i < mem->capacity; i++) {
        mem->hash_table[i] = NULL;
    }
    return mem;
}

// Adds new_value into structure, or if it exists it modifies it
void sync_info_mem_store_add(sync_info_mem_store* mem,dir_info new_value) {
    // New value needs to be saved in chain hash_table[pos]
    int pos = hash_code(new_value.target_dir) % mem->capacity;
    // We either create the chain or add it to the chain
    if (mem->hash_table[pos] == NULL) {
        mem->hash_table[pos] = lnode_create(new_value, NULL);
    }
    else {
        lnode_add_next(mem->hash_table[pos],new_value);
    }
    mem->size++;
    // Checking for rehash, if necessary
    if ((mem->size / (double)mem->capacity) > LOAD_FACTOR) {
        sync_info_mem_store_rehash(mem);
    }
}


// Removes value with given source_dir
void sync_info_mem_store_remove(sync_info_mem_store* mem,char* source_dir) {
    int pos = hash_code(source_dir) % mem->capacity;
    lnode_remove(&(mem->hash_table[pos]),source_dir);
}

// Returns status of the source dir or NULL if it doesn't exist
dir_info* sync_info_mem_store_find(sync_info_mem_store* mem,char* source_dir) {
    int pos = hash_code(source_dir) % mem->capacity;
    return lnode_find(mem->hash_table[pos], source_dir);
}

// Frees the structure from the memory 
void sync_info_mem_store_destroy(sync_info_mem_store* mem) {
    for (int i = 0; i < mem->capacity; i++) {
        // We destroy all the chains we have created
        if (mem->hash_table[i] != NULL) {
            lnode_destroy(&(mem->hash_table[i]));
        }
    }
    free(mem->hash_table);
    free(mem);
}

void sync_info_mem_store_rehash(sync_info_mem_store* mem) {
    // Selecting a new capacity from primes array 
    int i;
    i = 0;
    int old_capacity = mem->capacity;
    // FIX double when PRIME_SIZES ends
    int new_capacity = mem->capacity;
    while (mem->capacity >= PRIME_SIZES[i]) {
        if (PRIME_SIZES[i] == 1610612741) {
            new_capacity = 2 * mem->capacity;
            break;
        }        
        new_capacity = PRIME_SIZES[i];
        i++;
    }
    mem->capacity = new_capacity;
    lnode** old_hash_table = mem->hash_table;
    mem->size = 0;
    mem->hash_table = malloc(sizeof(lnode*) * mem->capacity);
    // We put all the values from the old chains to our new hash table
    //  and then we free the space we allocated for old hash_table
    for (int i = 0; i < old_capacity; i++) {
        while (old_hash_table[i] != NULL) {
            sync_info_mem_store_add(mem, lnode_get_value(old_hash_table[i]));
            lnode_remove_head(&(old_hash_table[i]));
        }
    }
    free(old_hash_table);
}

// We use the dbj2 hash function
uint hash_code(char* value) {
    uint hash = 5381;
    int i = 0;
    while (value[i] != '\0') {
        hash = (hash * 33) + value[i++];
    }
    return i;
}
