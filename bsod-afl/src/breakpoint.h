#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <stdbool.h>

struct node {
    unsigned long address;
    unsigned long taken_addr;
    unsigned long not_taken_addr;
    bool taken;
    bool not_taken;
    unsigned char cf_backup;
    struct node *next;
};

struct table {
    int size;
    struct node **nodes;
};

struct table *create_table(int size);
struct node *get_address(struct table *t, unsigned long address);
void insert_breakpoint(struct table *t, unsigned long address, unsigned long taken_addr,
                       unsigned long not_taken_addr, unsigned char cf_backup);

#endif