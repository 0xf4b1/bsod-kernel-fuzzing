#include <stdbool.h>

struct breakpoint {
    unsigned long address;
    unsigned long taken_addr;
    unsigned long not_taken_addr;
    bool taken;
    bool not_taken;
    unsigned char cf_backup;
    unsigned char cf_backup_taken;
    unsigned char cf_backup_not_taken;
    int module_id;
};

struct node {
    struct breakpoint *breakpoint;
    struct node *next;
};

struct table {
    int size;
    struct node **nodes;
};

struct table *create_table(int size);
struct breakpoint *get_breakpoint(struct table *t, unsigned long address);
void insert_breakpoint(struct table *t, int module_id, unsigned long address,
                       unsigned long taken_addr, unsigned long not_taken_addr,
                       unsigned char cf_backup, unsigned char cf_backup_taken,
                       unsigned char cf_backup_not_taken);
