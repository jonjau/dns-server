/*
 * COMP30023 Project 1
 * Author: Jonathan Jauhari 1038331
 * 
 * Contains content created for COMP20007 Design of Algorithms 2017
 * by Matt Farrugia <matt.farrugia@unimelb.edu.au>
 * updated in 2020 by Tobias Edwards <tobias.edwards@unimelb.ed.ua>
 *
 * List module containing functions for manipulation of singly-linked lists,
 * specialised to store processes in the context of process scheduling.
 */

#ifndef LIST_H
#define LIST_H

#include "cache_entry.h"

/**
 * The linked list provided contains processes
 */
typedef cache_entry_t *data_t;

/**
 * Singly linked list node
 */
typedef struct node node_t;
struct node {
    data_t data;
    node_t *next;
};

/**
 * Singly linked list
 */
typedef struct list list_t;
struct list {
    node_t *head;
    node_t *tail;
    int size;
};

list_t *copy_list(list_t *list);

/**
 * Creates a new empty list and returns it.
 */
list_t *new_list();

/**
 * Frees a list (including its nodes) and free its memory.
 */
void free_list(list_t *list);

/**
 * Returns the node that contains the minimum data O(n), based on the
 * comparison function provided. If such a node is not found, the program
 * exits.
 */
data_t list_min(list_t *list, int (*cmp)(data_t, data_t));

/**
 * Removes the first node that contains data equal to the given data, O(n),
 * If such a node is not found, this is a no-op and the function returns NULL
 */
data_t list_remove(list_t *list, data_t data);

/**
 * Removes the node that contains the minimum data O(n), based on the
 * comparison function provided. If such a node is not found, the program
 * exits.
 */
data_t list_remove_min(list_t *list, int (*cmp)(data_t, data_t));

/**
 * Removes and returns an element from the start of a list, O(1)
 */
data_t list_remove_start(list_t *list);

/**
 * Adds an element to the start of a list, O(1)
 */
void list_add_start(list_t *list, data_t data);

/**
 * Adds an element to the back of a list, O(1)
 */
void list_add_end(list_t *list, data_t data);

/**
 * Returns the number of elements contained in a list.
 */
int list_size(list_t *list);

/**
 * Returns whether the list contains no elements (true).
 */
bool list_is_empty(list_t *list);

/**
 * Returns the data in the head of the given list.
 */
data_t list_head(list_t *list);

/**
 * Returns the data in the tail of the given list.
 */
data_t list_tail(list_t *list);

/**
 * Mergesort the given linked list recursively
 */
void mergesort(list_t *list, int (*cmp)(data_t, data_t));

#endif
