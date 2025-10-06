/** Header file for lnode.c. lnode is a linked-list structure
 * that contains dir_info values and it is used by sync_info_mem_store 
 * iternally 
 **/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#pragma once

typedef struct linked_node lnode;

typedef struct {
    char source_dir[128]; // Our source directory
    char target_dir[128]; // Our target directory
    char status[128]; // The status of the last operation (SUCESS,PARTIAL,ERROR)
    char last_sync_time[128]; // Last time that it was synced
    int active_error_count; // Number of errors happened
    bool is_active; // Is true if the directory is actively monitored 
    bool is_in_workers; // Is true if a worker is trying to sync the directory
                        //  or if it's in queue
} dir_info;

struct linked_node {
    dir_info value;
    lnode* next;
};

// Creates a lnode and returns a pointer to it
lnode* lnode_create(dir_info value,lnode* next);

// Setters
void lnode_set_value(lnode* node,dir_info new_value);
void lnode_set_next(lnode* node,lnode* next);

// Removes head node
void lnode_remove_head(lnode **node);

// Getters
dir_info lnode_get_value(lnode* node);
lnode* lnode_get_next(lnode* node);

// Adds a node next to head
void lnode_add_next(lnode* node,dir_info new_value);

// Removes node with given value
void lnode_remove(lnode** node,char* source_dir);

// Returns the directory information of source_dir or NULL if not found
dir_info* lnode_find(lnode* node,char* source_dir);

// Frees lnode from memory
void lnode_destroy(lnode** node);

