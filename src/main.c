/**
 * =====================================================================================
 *
 * Filename:  main.c
 *
 * Description:  Ponto de entrada do logger de chamadas de sistema.
 * Responsável por iniciar e controlar o processo alvo usando ptrace.
 *
 * Team:  [NOME DA EQUIPE AQUI]
 * * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>  // Obrigatório para a struct user_regs_struct
#include <sys/prctl.h> // Obrigatório para prctl(PR_SET_PDEATHSIG)
#include <signal.h>    // Obrigatório para SIGKILL

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

        // Loop principal: vamos capturar cada syscall
        while (1)
        {
            // Prepara para a ENTRADA da syscall
            wait_for_syscall(child_pid);

            // Estrutura para armazenar os registradores da CPU do processo filho
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);

            /// TODO: Chamar a função de `parser.c` aqui para decodificar `regs`
            // Por enquanto, vamos apenas imprimir o número da syscall (que fica no registrador orig_rax em x86-64)
            printf("SYSCALL_ENTRY: Número = %lld\n", regs.orig_rax);

            // Prepara para a SAÍDA da syscall
            wait_for_syscall(child_pid);

            // Pega os registradores novamente para ver o valor de retorno
            ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);

            /// TODO: Capturar e formatar o valor de retorno, que fica no registrador `rax`
            printf("SYSCALL_EXIT:  Retorno = %lld\n\n", regs.rax);
        }
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
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
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
