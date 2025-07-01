/**
 * =====================================================================================
 *
 * Filename:  main.c
 *
 * Description:  Ponto de entrada do logger de chamadas de sistema.
 * Responsável por iniciar e controlar o processo alvo usando ptrace.
 *
 * Team:  Sérgio, Joel, Gustavo e Vinícius
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

#ifdef __linux__
#include <sys/prctl.h>  // Específico do Linux
#endif

// Inclui nosso módulo de parsing
#include "parser.h"


// --- Variáveis Globais ---
FILE *log_file = NULL;  // arquivo de log global

// --- Protótipos de Funções ---
void wait_for_syscall(pid_t child_pid);
void open_log_file();
void close_log_file();
void sigint_handler(int sig);

/**
 * @brief Ponto de entrada principal do programa.
 */
int main(int argc, char *argv[])
{
    // Registra nosso manipulador para o sinal SIGINT (Ctrl+C)
    signal(SIGINT, sigint_handler);

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
        // --- Processo Filho (o "Tracee") ---

        // 1. Permite que o processo pai o rastreie.
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);

        // 2. (Opcional, mas boa prática) Pede ao kernel para matar o filho se o pai morrer.
        //    Isso evita processos "órfãos" se nosso logger crashar.
        #ifdef __linux__
        prctl(PR_SET_PDEATHSIG, SIGKILL);
        #endif

        // 3. Substitui a imagem do processo filho pelo comando que queremos monitorar.
        //    O sistema operacional vai parar o processo aqui e notificar o pai (por causa do PTRACE_TRACEME).
        execvp(argv[1], &argv[1]);

        // Se execvp() retornar, significa que deu erro.
        perror("execvp");
        exit(1);
    }
    else
    {
        // --- Processo Pai (o "Tracer") ---

        printf("[*] Iniciando tracer para o processo filho com PID: %d\n", child_pid);
        printf("[*] Comando: %s\n\n", argv[1]);
        printf("[*] Pressione Ctrl+C para parar o rastreamento e salvar o log.\n\n");

        open_log_file();

        // Espera o filho parar na chamada execvp()
        wait(NULL);
        ptrace(PTRACE_SETOPTIONS, child_pid, NULL, PTRACE_O_TRACESYSGOOD);

        // Loop principal: vamos capturar cada syscall
        while(1) {
             // Estrutura para armazenar os registradores da CPU do processo filho.
            // Declaramos uma vez aqui, e cada bloco a preenche.
            struct user_regs_struct regs;
            long long syscall_number;
            long long return_value;

            // --- TRATAMENTO DA ENTRADA DA SYSCALL ---
            wait_for_syscall(child_pid);

            #if defined(__x86_64__)
                ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
                syscall_number = regs.orig_rax;
            #elif defined(__aarch64__)
                struct iovec iov = { .iov_base = &regs, .iov_len = sizeof(regs) };
                ptrace(PTRACE_GETREGSET, child_pid, NT_PRSTATUS, &iov);
                syscall_number = regs.regs[8];
            #else
                #error "Arquitetura não suportada."
            #endif

            // Agora com o número da syscall, chamamos o parser
            const char *syscall_name = get_syscall_name(syscall_number);
            
            // A função do parser agora cuida de TODO o logging (arquivo e console)
            log_syscall_args(child_pid, &regs, syscall_name, log_file);


            // --- TRATAMENTO DA SAÍDA DA SYSCALL ---
            wait_for_syscall(child_pid);

            #if defined(__x86_64__)
                ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
                return_value = regs.rax;
            #elif defined(__aarch64__)
                iov.iov_base = &regs;
                iov.iov_len = sizeof(regs);
                ptrace(PTRACE_GETREGSET, child_pid, NT_PRSTATUS, &iov);
                return_value = regs.regs[0];
            #endif

            // Loga o valor de retorno no arquivo e no console
            fprintf(log_file, "  -> Retorno = %lld\n\n", return_value);
            printf("  -> Retorno = %lld\n\n", return_value);
            fflush(log_file);
               
        } // Fim do while(1)
    }

    return 0;
}

/**
 * @brief Avança o processo filho até a próxima entrada/saída de syscall e espera.
 * * @param child_pid O PID do processo filho a ser monitorado.
 */
void wait_for_syscall(pid_t child_pid)
{
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
            return; // Parou em uma syscall, retorna para o loop principal
        }

        // WIFEXITED: Verifica se o processo filho terminou sua execução.
        if (WIFEXITED(status))
        {
            printf("\n[*] Processo filho terminou.\n");
            close_log_file(); // Fecha o log antes de sair
            exit(0);
        }
    }
}

/**
 * @brief Abre o arquivo de log para escrita.
 */
void open_log_file() 
{
    log_file = fopen("syscall_log.txt", "w");
    if (!log_file) {
        perror("Erro ao abrir arquivo de log");
        exit(1);
    }
    fprintf(log_file, "--- Início do Log de Chamadas de Sistema ---\n\n");
}

/**
 * @brief Fecha o arquivo de log de forma segura.
 */
void close_log_file() {
    if (log_file) {
        fprintf(log_file, "\n--- Fim do Log ---\n");
        fclose(log_file);
        log_file = NULL; // Evita double-free
    }
}

/**
 * @brief Manipulador para o sinal SIGINT (Ctrl+C).
 */
void sigint_handler(int sig) {
    (void)sig; // Evita warning de "unused parameter"
    printf("\n[*] Sinal de interrupção recebido. Encerrando de forma limpa...\n");
    close_log_file();
    exit(0);
}
