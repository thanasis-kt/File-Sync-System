#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "hash_table.h"
#include "list_node.h"

// Using primes as size, for the same reasons as in sync_info_mem_store.c (they are both hash tables)
int HASH_TABLE_PRIME_SIZES[] = {53, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241,
	786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653, 100663319, 201326611, 402653189, 805306457, 1610612741};

// Returns the new capacity we must chose when we rehash, to improve efficiency
int find_next_capacity(int a);

// Creates a hash table dynamically and returns a pointer to it
hash_table* hash_table_create(void) {
    hash_table* map = malloc(sizeof(hash_table));
    if (map == NULL) {
        perror("ERROR! malloc failed\n");
        exit(-1);
    }
    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    map->table = malloc(sizeof(list_node*) * map->capacity);
    if (map->table == NULL) {
        perror("ERROR! malloc failed\n");
        exit(-1);
    }
    for (int i = 0; i < map->capacity; i++) {
        map->table[i] = NULL;
    }
    return map;
}

// Inserts a pair into the hash table
void hash_table_insert(hash_table* map,int key,char* value) {
    // To avoid duplicate values
    if (hash_table_find(map,key) != NULL)
        return;

    int pos = (uint)key % map->capacity;
    if (map->table[pos] == NULL) {
        map->table[pos] = list_node_create(key,value,NULL);
    }
    else 
        list_node_add(map->table[pos],key,value);
}

// Returns the pair in the hash table that has the given key, or NULL if it 
// doesn't exist
pair* hash_table_find(hash_table* map,int key) {
    int pos = (uint)key % map->capacity;
    return list_node_find(map->table[pos],key);
}

// Removes the pair with key from the map, if it exists
void hash_table_remove(hash_table* map,int key) {
    int pos = key % map->capacity;
    list_node_remove(&(map->table[pos]),key);
}
 
// Removes all values from map (doesn't free map itself)
void hash_table_destroy(hash_table* map) {
    for (int i = 0; i < map->capacity; i++) {
        list_node_destroy(&(map->table[i]));
    }
}

// This function will be used for rehashing our table
void hash_table_rehash(hash_table* map) {
    list_node** old_array = map->table;
    int old_capacity = map->capacity;
    // Find next capacity for our map
    map->capacity = find_next_capacity(map->capacity);
    map->table = malloc(sizeof(list_node*) * map->capacity);
    for (int i = 0; i < old_capacity; i++) {
        list_node* node = old_array[i];
        while(node != NULL) {
            hash_table_insert(map,node->value.key,node->value.value);
            list_node_remove(&node,node->value.key);
        }
    }
    free(old_array);
}

int hash_table_find_key(hash_table* map,char* value) {
    for (int i = 0; i < map->capacity; i++) {
        list_node* node = map->table[i];
        while (node != NULL) {
            if (strcmp(value,node->value.value) == 0)
                return node->value.key;
            node = node->next;
        }    
    }
    return -1;
}


int find_next_capacity(int a) {
    int i = 0;
    while (true) {
        // If we found a bigger prime than a we return it
        if (HASH_TABLE_PRIME_SIZES[i] > a)
            return HASH_TABLE_PRIME_SIZES[i];
        if (a > 1610612741)  
            break;
        i++;
    }
    //a is bigger than any of our current primes
    // We choose 2 * a + 1 to attemp to reduse clusters
    // from 2 * a

    return 2 * a + 1; }
