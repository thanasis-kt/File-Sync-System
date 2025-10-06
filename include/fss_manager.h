/* Header file for fss_manager program */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include "sync_info_mem_store.h"
#include "queue.h"
#define DEFAULT_WORKERS_LIMIT 5

#pragma once

// Our signal handler 
void handler(int sig);

void get_workers_input(struct pollfd* fds,int size);

// Returns true if sub is substring of str
bool is_substring(char* str,char* sub);

// Returns true if buffer contains '\n' character. Buffer must be NULL 
// terminated
bool is_full_command(char* buffer);

// Copies the current time in time_buffer and returns it. Doesn't alocate 
// time_buffer. Time is in the form "%Y-%m-%d %H:%M:%S"
char* print_timestamp(char time_buffer[32]);

// Prints workers output in form: 
// [TIMESTAMP] [SOURCE_DIR] [TARGET_DIR] [WORKER_PID] [OPERATION] [RESULT] [DETAILS]
void print_workers_output(FILE* output,char* buffer,int worker_pid);


void extract_workers_output(char* buffer,char** source_dir,char** target_dir,char** status,char** operation,char** details,int* errors_found);
