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

#include "list.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

node_t *new_node(data_t data);
void free_node(node_t *node);

void mergesort_rec(node_t **head_ref, int (*cmp)(data_t, data_t));
node_t *sorted_merge(node_t *a, node_t *b, int (*cmp)(data_t, data_t));
void split(node_t *source, node_t **front, node_t **back);

/****************************************************************************/

/**
 * Adapted from content created for COMP20007 Design of Algorithms 2017
 * by Matt Farrugia <matt.farrugia@unimelb.edu.au>
 * updated in 2020 by Tobias Edwards <tobias.edwards@unimelb.ed.ua>
 * Edited by Jonathan Jauhari 1038331 <jjauhari@student.unimelb.edu.au>
 */

/**
 * Creates a new empty list and returns it.
 */
list_t *new_list() {
    list_t *list = malloc(sizeof(*list));
    assert(list);

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;

    return list;
}

/**
 * Creates a list that is a shallow copy of `list` and returns it
 */
list_t *copy_list(list_t *list) {
    list_t *list2 = new_list();
    node_t *curr;
    for (curr = list->head; curr; curr = curr->next) {
        list_add_end(list2, curr->data);
    }
    return list2;
}

/**
 * Frees a list (including its nodes) and free its memory.
 */
void free_list(list_t *list) {
    assert(list);
    node_t *node = list->head;
    node_t *next;
    while (node) {
        next = node->next;
        free_node(node);
        node = next;
    }
    free(list);
}

/**
 * Creates a new node and returns it.
 */
node_t *new_node(data_t data) {
    node_t *node = malloc(sizeof(*node));
    assert(node);
    node->next = NULL;
    node->data = data;
    return node;
}

/**
 * Clears memory of a given node.
 */
void free_node(node_t *node) { free(node); }

/**
 * Removes the first node that contains data equal to the given data, O(n),
 * If such a node is not found, this is a no-op and the function returns NULL
 */
data_t list_remove(list_t *list, data_t data) {
    assert(list);
    if (data == NULL || list->size == 0) {
        return NULL;
    }
    node_t *target_prev = NULL;
    node_t *target_node = list->head;
    node_t *prev = list->head;
    node_t *node = list->head->next;

    /* find node matching the data */
    while (node != NULL) {
        if (cache_entry_eq(node->data, data)) {
            target_prev = prev;
            target_node = node;
            break;
        }
        prev = node;
        node = node->next;
    }
    if (!cache_entry_eq(target_node->data, data)) {
        return NULL;
    }
    data_t target = target_node->data;

    /* If we're deleting head or the tail then we better update that
     * we're removing this node
     */
    if (target_node == list->head) {
        list->head = list->head->next;
    } else {
        assert(target_prev != NULL);
        assert(target_prev->next == target_node);
        target_prev->next = target_node->next;
    }
    if (target_node == list->tail) {
        list->tail = target_prev;
    }
    list->size--;
    free_node(target_node);
    return target;
}

/**
 * Removes the node that contains the minimum data O(n) and returns it.
 * If such a node is not found, the program exits.
 */
data_t list_remove_min(list_t *list, int (*cmp)(data_t, data_t)) {
    data_t min = list_min(list, cmp);
    return list_remove(list, min);
}

/**
 * Returns the node that contains the minimum data O(n), based on the
 * comparison function provided. If such a node is not found, the program
 * exits.
 */
data_t list_min(list_t *list, int (*cmp)(data_t, data_t)) {
    assert(list && list->size > 0);
    node_t *curr = list->head;
    data_t min = list->head->data;
    while (curr != NULL) {
        if (cmp(curr->data, min) < 0) {
            min = curr->data;
        }
        curr = curr->next;
    }
    return min;
}

/**
 * Removes and returns an element from the start of a list, O(1)
 */
data_t list_remove_start(list_t *list) {
    assert(list);
    if (list->size == 0) {
        exit(EXIT_FAILURE);
    }

    node_t *old_head = list->head;
    data_t data = old_head->data;

    list->head = old_head->next;

    /* remove last element, otherwise, replace head with the next node */
    if (list->size == 1) {
        list->head = NULL;
    }
    list->size--;
    free(old_head);
    return data;
}

/**
 * Adds an element to the start of a list, O(1)
 */
void list_add_start(list_t *list, data_t data) {
    assert(list);
    node_t *node = new_node(data);
    node->next = list->head;
    list->head = node;

    if (list->size == 0) {
        list->tail = node;
    }
    list->size++;
}

/**
 * Adds an element to the back of a list, O(1)
 */
void list_add_end(list_t *list, data_t data) {
    assert(list);
    node_t *node = new_node(data);
    node->next = NULL;
    if (list->size == 0) {
        list->head = node;
    } else {
        list->tail->next = node;
    }
    list->tail = node;
    list->size++;
}

/**
 * Returns the number of elements contained in a list.
 */
int list_size(list_t *list) {
    assert(list);
    return list->size;
}

/**
 * Returns whether the list contains no elements (true).
 */
bool list_is_empty(list_t *list) {
    assert(list);
    return (list->size == 0);
}

/**
 * Returns the data in the head of the given list.
 */
data_t list_head(list_t *list) {
    assert(list != NULL);
    return list->head->data;
}

/**
 * Returns the data in the tail of the given list.
 */
data_t list_tail(list_t *list) {
    assert(list != NULL);
    return list->tail->data;
}

/****************************************************************************/

/**
 * Mergesort the given linked list recursively
 */
void mergesort(list_t *list, int (*cmp)(data_t, data_t)) {
    mergesort_rec(&list->head, cmp);
}

/**
 * Helper function: Mergesort the given linked list recursively
 */
void mergesort_rec(node_t **head_ref, int (*cmp)(data_t, data_t)) {
    node_t *head = *head_ref;
    node_t *a;
    node_t *b;

    if ((head == NULL) || (head->next == NULL)) {
        return;
    }
    split(head, &a, &b);

    /* recursively sort the sublists */
    mergesort_rec(&a, cmp);
    mergesort_rec(&b, cmp);

    /* answer = merge the two sorted lists together */
    *head_ref = sorted_merge(a, b, cmp);
}

/**
 * Merge the two list fragments `a` and `b`, in sorted order, into one
 * and return it.
 */
node_t *sorted_merge(node_t *a, node_t *b, int (*cmp)(data_t, data_t)) {
    node_t *result = NULL;
    if (a == NULL) {
        return b;
    } else if (b == NULL) {
        return a;
    }
    /* pick either a or b, and recur */
    if (cmp(a->data, b->data) < 0) {
        result = a;
        result->next = sorted_merge(a->next, b, cmp);
    } else {
        result = b;
        result->next = sorted_merge(a, b->next, cmp);
    }
    return result;
}

/**
 * Split a list fragment `source` into two halves, each half is stored as
 * list fragments in `front_ref` and `back_ref`.
 */
void split(node_t *source, node_t **front_ref, node_t **back_ref) {
    node_t *slow = source;
    node_t *fast = source->next;

    /* advance 'fast' two nodes, and advance 'slow' one node */
    while (fast != NULL) {
        fast = fast->next;
        if (fast != NULL) {
            slow = slow->next;
            fast = fast->next;
        }
    }
    /* split source in two where slow is, that is the midpoint. */
    *front_ref = source;
    *back_ref = slow->next;
    slow->next = NULL;
}
