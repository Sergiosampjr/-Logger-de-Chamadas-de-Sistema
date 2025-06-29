/**
 * =====================================================================================
 *
 * Filename:  main.c
 *
 * Description:  Ponto de entrada do logger de chamadas de sistema.
 * Responsável por iniciar e controlar o processo alvo usando ptrace.
 *
 * Team:  Sérgio Nunes,Joel Lacerda,Gustavo e Vinícius
 * * =====================================================================================
 */
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>   // Obrigatório para a struct user_regs_struct
#include <sys/prctl.h>  // Obrigatório para prctl(PR_SET_PDEATHSIG)
#include <signal.h>     // Obrigatório para SIGKILL
#include <sys/uio.h>    // Obrigatório para a struct iovec
#include <linux/elf.h>  // Obrigatório para a constante NT_PRSTATUS
#include <stdint.h>
#include <time.h>  // para timestamp
#include <sys/types.h>



FILE *log_file = NULL;  // arquivo de log global


void open_log_file() {
    log_file = fopen("syscall_log.txt", "w");
    if (!log_file) {
        perror("Erro ao abrir arquivo de log");
        exit(1);
    }
}


void close_log_file() {
    if (log_file) {
        fclose(log_file);
    }
}





// --- Assinaturas de Funções ---
// #include "parser.h" // Vamos descomentar isso quando parser.h for criado.
void wait_for_syscall(pid_t child_pid);

/**
 * @brief Ponto de entrada principal do programa.
 */




 
int main(int argc, char *argv[])
{
    // Valida se o usuário passou um comando para ser executado.
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <comando para executar>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s /bin/ls -l\n", argv[0]);
        return 1;
    }

    // Usa fork() para criar um novo processo.
    pid_t child_pid = fork();

    if (child_pid == -1)
    {
        perror("fork"); // Se fork() falhar.
        return 1;
    }

    if (child_pid == 0)
    {
        // --- Estamos no Processo Filho (o "Tracee") ---

        // 1. Permite que o processo pai o rastreie.
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        // 2. (Opcional, mas boa prática) Pede ao kernel para matar o filho se o pai morrer.
        //    Isso evita processos "órfãos" se nosso logger crashar.
        prctl(PR_SET_PDEATHSIG, SIGKILL);

        // 3. Substitui a imagem do processo filho pelo comando que queremos monitorar.
        //    O sistema operacional vai parar o processo aqui e notificar o pai (por causa do PTRACE_TRACEME).
        execvp(argv[1], &argv[1]);

        // Se execvp() retornar, significa que deu erro.
        perror("execvp");
        exit(1);
    }
    else
    {
        // --- Estamos no Processo Pai (o "Tracer") ---

        printf("[*] Iniciando tracer para o processo filho com PID: %d\n", child_pid);
        printf("[*] Comando: %s\n\n", argv[1]);

        // Espera o filho parar na chamada execvp()
        wait(NULL);
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);

        open_log_file();
         // Loop principal: vamos capturar cada syscall
        while(1) {
            // Estrutura para armazenar os registradores da CPU do processo filho.
            // Declaramos uma vez aqui, e cada bloco a preenche.
            struct user_regs_struct regs;

            // --- TRATAMENTO DA ENTRADA DA SYSCALL ---
            wait_for_syscall(child_pid);

            #if defined(__x86_64__)
                // Lógica específica para x86-64 para obter os registradores
                ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
                const char *syscall_name = get_syscall_name(regs.orig_rax);
                printf("SYSCALL_ENTRY: %s (%lld)\n", syscall_name, regs.orig_rax);

                // Aqui você chama a função que imprime os argumentos da syscall:
                log_syscall_args(child_pid, &regs, syscall_name, log_file);

                // --- TRATAMENTO DA SAÍDA DA SYSCALL ---
                wait_for_syscall(child_pid);

                // Pega os registradores novamente para obter o valor de retorno
                ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
                printf("SYSCALL_EXIT:  Retorno = %lld\n\n", regs.rax);

            #elif defined(__aarch64__)
                // Lógica específica para aarch64 para obter os registradores
                struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
                ptrace(PTRACE_GETREGSET, child_pid, NT_PRSTATUS, &iov);
                printf("SYSCALL_ENTRY: Número = %lld\n", regs.regs[8]);

                // --- TRATAMENTO DA SAÍDA DA SYSCALL ---
                wait_for_syscall(child_pid);

                // Pega os registradores novamente para obter o valor de retorno
                ptrace(PTRACE_GETREGSET, child_pid, NT_PRSTATUS, &iov);
                printf("SYSCALL_EXIT:  Retorno = %lld\n\n", regs.regs[0]);

            #else
                #error "Arquitetura não suportada."
            #endif
        } // Fim do while(1)
        close_log_file();
    }

    return 0;
}

/**
 * @brief Avança o processo filho até a próxima entrada/saída de syscall e espera.
 * * @param child_pid O PID do processo filho a ser monitorado.
 */
void wait_for_syscall(pid_t child_pid){
    int status;
    while (1)
    {
        // Continua o processo filho, mas para na próxima chamada de sistema.
        ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);

        // Espera pelo filho.
        waitpid(child_pid, &status, 0);

        // WIFSTOPPED: Verifica se o filho parou por um sinal.
        // WSTOPSIG: Pega o sinal que parou o filho.
        // Queremos continuar apenas se o sinal for de uma trap de syscall (SIGTRAP).
        if (WIFSTOPPED(status) && (WSTOPSIG(status) & 0x80))
        {
            return;
        }

        // WIFEXITED: Verifica se o processo filho terminou sua execução.
        if (WIFEXITED(status))
        {
            printf("\n[*] Processo filho terminou.\n");
            exit(0);
        }
    }
}
