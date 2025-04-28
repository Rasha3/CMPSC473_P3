#include "linked_list.h"

// Creates and returns a new list
list_t* list_create() {
    list_t *list = (list_t *)malloc(sizeof(list_t));
    if (list) {
        list->head = NULL;
        list->count = 0;
    }
    return list;
}

// Destroys a list
void list_destroy(list_t* list) {
    list_node_t *current = list->head;
    while (current != NULL) {
        list_node_t *next = current->next;
        free(current);
        current = next;
    }
    free(list);
}

// Returns beginning of the list
list_node_t* list_begin(list_t* list) {
    return list->head;
}

// Returns next element in the list
list_node_t* list_next(list_node_t* node) {
    return node->next;
}

// Returns data in the given list node
void* list_data(list_node_t* node) {
    return node->data;
}

// Returns the number of elements in the list
size_t list_count(list_t* list) {
    return list->count;
}

// Finds the first node in the list with the given data
list_node_t* list_find(list_t* list, void* data) {
    list_node_t *current = list->head;
    while (current != NULL) {
        if (current->data == data) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Inserts a new node in the list with the given data
void list_insert(list_t* list, void* data) {
    list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
    if (new_node) {
        new_node->data = data;
        new_node->next = list->head;
        new_node->prev = NULL;
        if (list->head != NULL) {
            list->head->prev = new_node;
        }
        list->head = new_node;
        list->count++;
    }
}

// Removes a node from the list and frees the node resources
void list_remove(list_t* list, list_node_t* node) {
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        list->head = node->next;
    }
    if (node->next != NULL) {
        node->next->prev = node->prev;
    }
    free(node);
    list->count--;
}

// Executes a function for each element in the list
void list_foreach(list_t* list, void (*func)(void* data)) {
    list_node_t *current = list->head;
    while (current != NULL) {
        func(current->data);
        current = current->next;
    }
}

