#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/list_node.h"

// Creates a list node dynamically and returns a pointer to it
list_node* list_node_create(int key,char* new_value,list_node* next) {
    list_node* lst = malloc(sizeof(list_node));
    if (lst == NULL){
        perror("ERROR! malloc failed\n");
        exit(-1);
    }
    lst->value.key = key;
    lst->value.value = malloc(sizeof(char) * (strlen(new_value) + 1));
    strcpy(lst->value.value,new_value);
    return lst;
}

// Adds new_value to the list
void list_node_add(list_node* lst,int key,char* new_value) {
    list_node* temp = list_node_create(key, new_value, lst->next);
    lst->next = temp;
}

// Removes value with given key from the key
void list_node_remove(list_node** lst,int key) {
    while (*lst != NULL) {
        if ((*lst)->value.key == key) {
            //Remove the node
            list_node* temp = *lst;
            *lst = (*lst)->next;
            free(temp->value.value);
            free(temp);
        }
        lst = &((*lst)->next);
    }
}

// Returns the pair in the list_node that has the given key, or NULL if it 
// doesn't exist
pair* list_node_find(list_node* lst,int key) {
    while (lst != NULL) {
        if (lst->value.key == key) {
            return &(lst->value);
        }
        lst = lst->next;
    }
    return NULL;
}

// Frees list_node from memory (and it's values)
void list_node_destroy(list_node** lst) {
    while (*lst != NULL) {
        //Remove the node
        list_node* temp = *lst;
        *lst = (*lst)->next;
        free(temp->value.value);
        free(temp);
        lst = &((*lst)->next);
    }
}
