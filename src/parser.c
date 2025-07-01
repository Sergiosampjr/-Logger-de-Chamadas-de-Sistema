#include "parser.h"
#include <sys/user.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

const char* get_syscall_name(long syscall_number) {
    #if defined(__x86_64__)
        switch (syscall_number) {
            #include "x86_64_table.h"
             default: return "unknown_syscall";
        }
    #elif defined(__aarch64__)
        switch (syscall_number) {
            #include "aarch64_table.h"
            default: return "unknown_syscall";
        }
    #else
        #error "Arquitetura não suportada."
    #endif
}

void log_syscall_args(pid_t pid, struct user_regs_struct *regs, const char *syscall_name, FILE *log_file) {
    // Esta parte inicial é independente da arquitetura
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Imprime o cabeçalho no arquivo e no terminal
    fprintf(log_file, "[%s] [PID %d] Syscall: %s\n", timestamp, pid, syscall_name);
    printf("[%s] [PID %d] Syscall: %s\n", timestamp, pid, syscall_name);

    #if defined(__x86_64__)
        // Registradores de argumento para x86-64
        fprintf(log_file, "  arg1(rdi): %lld\n", (long long) regs->rdi);
        fprintf(log_file, "  arg2(rsi): %lld\n", (long long) regs->rsi);
        fprintf(log_file, "  arg3(rdx): %lld\n", (long long) regs->rdx);
        fprintf(log_file, "  arg4(r10): %lld\n", (long long) regs->r10);
        fprintf(log_file, "  arg5(r8):  %lld\n", (long long) regs->r8);
        fprintf(log_file, "  arg6(r9):  %lld\n", (long long) regs->r9);

        printf("  arg1(rdi): %lld\n", (long long) regs->rdi);
        printf("  arg2(rsi): %lld\n", (long long) regs->rsi);
        printf("  arg3(rdx): %lld\n", (long long) regs->rdx);
        printf("  arg4(r10): %lld\n", (long long) regs->r10);
        printf("  arg5(r8):  %lld\n", (long long) regs->r8);
        printf("  arg6(r9):  %lld\n", (long long) regs->r9);

    #elif defined(__aarch64__)
        // Registradores de argumento para aarch64 (ARM64)
        fprintf(log_file, "  arg1(x0): %lld\n", (long long) regs->regs[0]);
        fprintf(log_file, "  arg2(x1): %lld\n", (long long) regs->regs[1]);
        fprintf(log_file, "  arg3(x2): %lld\n", (long long) regs->regs[2]);
        fprintf(log_file, "  arg4(x3): %lld\n", (long long) regs->regs[3]);
        fprintf(log_file, "  arg5(x4): %lld\n", (long long) regs->regs[4]);
        fprintf(log_file, "  arg6(x5): %lld\n", (long long) regs->regs[5]);

        printf("  arg1(x0): %lld\n", (long long) regs->regs[0]);
        printf("  arg2(x1): %lld\n", (long long) regs->regs[1]);
        printf("  arg3(x2): %lld\n", (long long) regs->regs[2]);
        printf("  arg4(x3): %lld\n", (long long) regs->regs[3]);
        printf("  arg5(x4): %lld\n", (long long) regs->regs[4]);
        printf("  arg6(x5): %lld\n", (long long) regs->regs[5]);

    #endif

    fflush(log_file); // Garante que seja escrito no arquivo imediatamente
    // Adiciona uma linha em branco para separar as syscalls
    fprintf(log_file, "\n");
    printf("\n");
}
