#include <stdio.h>  // Para declarar FILE
#include <sys/types.h>
#include <sys/user.h>  

#ifndef PARSER_H
#define PARSER_H

// Para struct user_regs_struct

void log_syscall_args(pid_t pid, struct user_regs_struct *regs, const char *syscall_name,FILE *log_file);
const char* get_syscall_name(long syscall_number);

#endif
