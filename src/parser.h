#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>  

#ifndef PARSER_H
#define PARSER_H

// Para struct user_regs_struct
const char* get_syscall_name(long syscall_number);
void log_syscall_args(pid_t pid, struct user_regs_struct *regs, const char *syscall_name, FILE *log_file);
static void read_string_from_child(pid_t child, unsigned long addr, char *buffer, size_t max_size);
static void decode_openat_flags(long flags, char *buffer, size_t max_size);

#endif
