#include "parser.h"
#include <sys/user.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>





const char* get_syscall_name(long syscall_number) {
    switch (syscall_number) {
        case 0: return "read";
        case 1: return "write";
        case 2: return "open";
        case 3: return "close";
        case 4: return "stat";
        case 5: return "fstat";
        case 6: return "lstat";
        case 7: return "poll";
        case 8: return "lseek";
        case 9: return "mmap";
        case 10: return "mprotect";
        case 11: return "munmap";
        case 12: return "brk";
        case 21: return "access";
        case 57: return "fork";
        case 59: return "execve";
        case 60: return "exit";
        case 257: return "openat";
        // Adicione mais syscalls que aparecerem no output
        default: return "unknown_syscall";
    }
}



void log_syscall_args(pid_t pid, struct user_regs_struct *regs, const char *syscall_name, FILE *log_file) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] [PID %d] Syscall %s chamada com argumentos:\n", timestamp, pid, syscall_name);
    fprintf(log_file, "  arg1: %lld\n", (long long) regs->rdi);
    fprintf(log_file, "  arg2: %lld\n", (long long) regs->rsi);
    fprintf(log_file, "  arg3: %lld\n", (long long) regs->rdx);
    fprintf(log_file, "  arg4: %lld\n", (long long) regs->r10);
    fprintf(log_file, "  arg5: %lld\n", (long long) regs->r8);
    fprintf(log_file, "  arg6: %lld\n", (long long) regs->r9);
    fflush(log_file);  // garante que seja escrito no arquivo imediatamente

    // Também imprime no terminal se quiser
    printf("[PID %d] Syscall %s chamada com argumentos:\n", pid, syscall_name);
    printf("  arg1: %lld\n", (long long) regs->rdi);
    printf("  arg2: %lld\n", (long long) regs->rsi);
    printf("  arg3: %lld\n", (long long) regs->rdx);
    printf("  arg4: %lld\n", (long long) regs->r10);
    printf("  arg5: %lld\n", (long long) regs->r8);
    printf("  arg6: %lld\n", (long long) regs->r9);
}


