/**
 * COMP30023 Project 2
 * Author: Jonathan Jauhari 1038331
 * 
 * Contains content created for COMP20007 Design of Algorithms 2017
 * by Matt Farrugia <matt.farrugia@unimelb.edu.au>
 * updated in 2020 by Tobias Edwards <tobias.edwards@unimelb.ed.ua>
 *
 * List module containing functions for manipulation of singly-linked lists,
 * specialised to store cached resource records for a DNS server.
 */

#ifndef LIST_H
#define LIST_H

#include "cache_entry.h"

/**
 * The linked list provided contains cache entries
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
list_t *new_list();
void free_list(list_t *list);

data_t list_min(list_t *list, int (*cmp)(data_t, data_t));

data_t list_remove(list_t *list, data_t data);
data_t list_remove_min(list_t *list, int (*cmp)(data_t, data_t));
data_t list_remove_start(list_t *list);

void list_add_start(list_t *list, data_t data);
void list_add_end(list_t *list, data_t data);

int list_size(list_t *list);
bool list_is_empty(list_t *list);

data_t list_head(list_t *list);
data_t list_tail(list_t *list);

#endif
