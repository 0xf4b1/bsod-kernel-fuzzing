#ifndef PRIVATE_H
#define PRIVATE_H

#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include <capstone.h>

#include "afl.h"
#include "signals.h"
#include "tracer.h"
#include "vmi.h"

char *socket;
char *json;
FILE *input_file;
char *input_path;
size_t input_size;
size_t input_limit;
unsigned char *input;
bool afl;
bool debug;
addr_t address;
addr_t address_pa;
addr_t module_start;
addr_t start_offset;
addr_t target_offset;
unsigned long limit;

vmi_instance_t vmi;
int interrupted;
unsigned long tracer_counter;

uint8_t cc;
uint8_t start_byte;
uint8_t target_byte;

csh cs_handle;

enum coverage { DYNAMIC, FULL, BLOCK, EDGE };
enum coverage mode;
char *bp_file;
bool coverage_enabled;
bool trace_pid;
vmi_pid_t current_pid;
vmi_pid_t harness_pid;
FILE *coverage_fp;
bool failure;
bool waiting;
bool reconnect;

#endif
