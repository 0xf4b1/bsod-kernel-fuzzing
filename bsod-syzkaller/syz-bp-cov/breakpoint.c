#include <stdio.h>
#include <stdlib.h>

#include "breakpoint.h"

struct table *create_table(int size) {
    struct table *t = (struct table *)calloc(1, sizeof(struct table));
    t->size = size;
    t->nodes = (struct node **)calloc(size, sizeof(struct node *));
    return t;
}

struct breakpoint *get_breakpoint(struct table *t, unsigned long address) {
    int pos = address % t->size;
    struct node *node = t->nodes[pos];

    while (node) {
        if (node->breakpoint->address == address || node->breakpoint->taken_addr == address ||
            node->breakpoint->not_taken_addr == address)
            return node->breakpoint;

        node = node->next;
    }

    return NULL;
}

void insert_breakpoint(struct table *t, int module_id, unsigned long address,
                       unsigned long taken_addr, unsigned long not_taken_addr,
                       unsigned char cf_backup, unsigned char cf_backup_taken,
                       unsigned char cf_backup_not_taken) {

    struct breakpoint *breakpoint = (struct breakpoint *)calloc(1, sizeof(struct breakpoint));
    breakpoint->address = address;
    breakpoint->taken_addr = taken_addr;
    breakpoint->not_taken_addr = not_taken_addr;
    breakpoint->cf_backup = cf_backup;
    breakpoint->cf_backup_taken = cf_backup_taken;
    breakpoint->cf_backup_not_taken = cf_backup_not_taken;
    breakpoint->module_id = module_id;

    int pos = address % t->size;
    struct node *new_node = (struct node *)calloc(1, sizeof(struct node));

    new_node->breakpoint = breakpoint;
    new_node->next = t->nodes[pos];
    t->nodes[pos] = new_node;

    pos = taken_addr % t->size;
    new_node = (struct node *)calloc(1, sizeof(struct node));

    new_node->breakpoint = breakpoint;
    new_node->next = t->nodes[pos];
    t->nodes[pos] = new_node;

    pos = not_taken_addr % t->size;
    new_node = (struct node *)calloc(1, sizeof(struct node));

    new_node->breakpoint = breakpoint;
    new_node->next = t->nodes[pos];
    t->nodes[pos] = new_node;
}
