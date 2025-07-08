#define _GNU_SOURCE

#include "parser.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/user.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>


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
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[26];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    // Variáveis para armazenar os argumentos de forma agnóstica
    long arg1, arg2, arg3;

    // Extrai os 3 primeiros argumentos com base na arquitetura
    #if defined(__x86_64__)
        arg1 = regs->rdi;
        arg2 = regs->rsi;
        arg3 = regs->rdx;
    #elif defined(__aarch64__)
        arg1 = regs->regs[0];
        arg2 = regs->regs[1];
        arg3 = regs->regs[2];
    #endif

    // --- Roteador de Decodificação ---

    if (strcmp(syscall_name, "openat") == 0) {
        char path[256];
        char flags_str[256];
        read_string_from_child(pid, arg2, path, sizeof(path));
        decode_openat_flags(arg3, flags_str, sizeof(flags_str));

        // Decodifica o primeiro argumento 'dirfd'
        const char *dirfd_str = (arg1 == AT_FDCWD) ? "AT_FDCWD" : "fd";
        
        fprintf(log_file, "[%s] [PID %d] %s(%s, \"%s\", %s)", timestamp, pid, syscall_name, dirfd_str, path, flags_str);
        printf("[%s] [PID %d] %s(%s, \"%s\", %s)", timestamp, pid, syscall_name, dirfd_str, path, flags_str);

    } else if (strcmp(syscall_name, "write") == 0) {
        // arg1 = fd, arg2 = buffer (endereço), arg3 = count
        fprintf(log_file, "[%s] [PID %d] %s(fd=%ld, buf=0x%lx, count=%ld)", timestamp, pid, syscall_name, arg1, (unsigned long)arg2, arg3);
        printf("[%s] [PID %d] %s(fd=%ld, buf=0x%lx, count=%ld)", timestamp, pid, syscall_name, arg1, (unsigned long)arg2, arg3);

    } else if (strcmp(syscall_name, "read") == 0) {
        // Similar ao write
        fprintf(log_file, "[%s] [PID %d] %s(fd=%ld, buf=0x%lx, count=%ld)", timestamp, pid, syscall_name, arg1, (unsigned long)arg2, arg3);
        printf("[%s] [PID %d] %s(fd=%ld, buf=0x%lx, count=%ld)", timestamp, pid, syscall_name, arg1, (unsigned long)arg2, arg3);

    } else if (strcmp(syscall_name, "close") == 0) {
        // arg1 = fd
        fprintf(log_file, "[%s] [PID %d] %s(fd=%ld)", timestamp, pid, syscall_name, arg1);
        printf("[%s] [PID %d] %s(fd=%ld)", timestamp, pid, syscall_name, arg1);

    } else if (strcmp(syscall_name, "newfstatat") == 0) {
        // arg1 = dirfd, arg2 = pathname (string), arg3 = struct stat*
        char path[256];
        const char *dirfd_str = (arg1 == AT_FDCWD) ? "AT_FDCWD" : "fd";
        read_string_from_child(pid, arg2, path, sizeof(path));
        fprintf(log_file, "[%s] [PID %d] %s(%s, \"%s\", ...)", timestamp, pid, syscall_name, dirfd_str, path);
        printf("[%s] [PID %d] %s(%s, \"%s\", ...)", timestamp, pid, syscall_name, dirfd_str, path);

    } else {
        // Fallback: se for uma syscall não tratada, imprime o nome e os 3 primeiros args brutos
        fprintf(log_file, "[%s] [PID %d] %s(0x%lx, 0x%lx, 0x%lx, ...)", timestamp, pid, syscall_name, (unsigned long)arg1, (unsigned long)arg2, (unsigned long)arg3);
        printf("[%s] [PID %d] %s(0x%lx, 0x%lx, 0x%lx, ...)", timestamp, pid, syscall_name, (unsigned long)arg1, (unsigned long)arg2, (unsigned long)arg3);
    }
}

// Função para ler uma string terminada em '\0' da memória do processo filho.
static void read_string_from_child(pid_t child, unsigned long addr, char *buffer, size_t max_size) {
    size_t i = 0;
    union {
        long val;
        char chars[sizeof(long)];
    } data;

    while (i < max_size - 1) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i, NULL);
        if (data.val == -1) { // Erro ou fim da memória acessível
            buffer[i] = '\0';
            break;
        }
        for (size_t j = 0; j < sizeof(long); ++j, ++i) {
            if (i >= max_size - 1) break;
            buffer[i] = data.chars[j];
            if (data.chars[j] == '\0') {
                return; // Fim da string
            }
        }
    }
    buffer[max_size - 1] = '\0'; // Garante terminação nula
}

// Função auxiliar para decodificar as flags da openat
static void decode_openat_flags(long flags, char *buffer, size_t max_size) {
    buffer[0] = '\0';
    if ((flags & O_ACCMODE) == O_RDONLY) strcat(buffer, "O_RDONLY");
    else if ((flags & O_ACCMODE) == O_WRONLY) strcat(buffer, "O_WRONLY");
    else if ((flags & O_ACCMODE) == O_RDWR) strcat(buffer, "O_RDWR");

    if (flags & O_CREAT)   strcat(buffer, "|O_CREAT");
    if (flags & O_APPEND)  strcat(buffer, "|O_APPEND");
    if (flags & O_TRUNC)   strcat(buffer, "|O_TRUNC");
    if (flags & O_CLOEXEC) strcat(buffer, "|O_CLOEXEC");
    if (flags & O_DIRECTORY) strcat(buffer, "|O_DIRECTORY");
    // Adicione mais flags se necessário
}
