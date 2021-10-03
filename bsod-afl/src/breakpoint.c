#include <stdio.h>
#include <stdlib.h>

#include "breakpoint.h"

struct table *create_table(int size) {
    struct table *t = (struct table *)calloc(1, sizeof(struct table));
    t->size = size;
    t->nodes = (struct node **)calloc(size, sizeof(struct node *));
    return t;
}

struct node *get_address(struct table *t, unsigned long address) {
    int pos = address % t->size;
    struct node *node = t->nodes[pos];

    while (node) {
        if (node->address == address)
            return node;

        node = node->next;
    }

    return NULL;
}

void insert_breakpoint(struct table *t, unsigned long address, unsigned long taken_addr,
                       unsigned long not_taken_addr, unsigned char cf_backup) {
    int pos = address % t->size;
    struct node *node = t->nodes[pos];

    while (node) {
        if (node->address == address)
            return;

        node = node->next;
    }

    struct node *new_node = (struct node *)calloc(1, sizeof(struct node));
    new_node->address = address;
    new_node->taken_addr = taken_addr;
    new_node->not_taken_addr = not_taken_addr;
    new_node->cf_backup = cf_backup;
    new_node->next = t->nodes[pos];
    t->nodes[pos] = new_node;
}
