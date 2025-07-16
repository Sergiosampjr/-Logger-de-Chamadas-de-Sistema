Logger de Chamadas de Sistema (Syscall Logger)
![alt text](https://img.shields.io/badge/lang-pt--br-green.svg)
Projeto final desenvolvido para a disciplina de Sistemas Operacionais. Trata-se de uma ferramenta em linguagem C que utiliza a chamada de sistema ptrace para interceptar e registrar as syscalls de um processo em execução no ambiente Linux.
🎯 Objetivo do Projeto
O objetivo principal desta ferramenta é monitorar a interação entre um programa de espaço de usuário e o kernel do Linux. Para cada chamada de sistema interceptada, o logger registra as seguintes informações em um arquivo de log para análise posterior:
Timestamp: O momento exato em que a chamada foi capturada.
PID: O ID do processo que está sendo monitorado.
Nome da Syscall: O nome legível da chamada de sistema (ex: openat, write).
Argumentos: Os valores contidos nos seis primeiros registradores de argumentos (de rdi a r9 em x86_64).
Valor de Retorno: O valor retornado pela syscall após sua execução.
👨‍💻 Equipe de Desenvolvimento
Sérgio Nunes
Joel
Gustavo
Vinicius
🛠️ Tecnologias e Conceitos Utilizados
Linguagem: C
API do Kernel: ptrace, fork, execvp, waitpid
Sistema Operacional Alvo: Linux (Ubuntu)
Ferramentas de Compilação: make, gcc
⚙️ Como Compilar e Executar
Certifique-se de que você está em um ambiente Linux com gcc e make instalados.

1. Clonar o Repositório
git clone https://github.com/seu-usuario/Logger-de-Chamadas-de-Sistema.git
cd Logger-de-Chamadas-de-Sistema

2. Compilar o Projeto
O projeto utiliza um Makefile para automatizar o processo de compilação. Para compilar, execute o seguinte comando na raiz do projeto:
make

Este comando irá gerar o executável meu_logger dentro do diretório bin/.
3. Executar o Logger
A ferramenta é executada a partir da linha de comando, passando o programa a ser monitorado como argumento. A saída do logger é salva automaticamente no arquivo syscall_log.txt.

./bin/meu_logger <comando> [argumentos...]


Exemplos de Uso:

Monitorar o comando ls:

./bin/meu_logger ls -l

Monitorar o comando cat lendo um arquivo:

./bin/meu_logger cat README.md

Monitorar o comando ping (pode exigir sudo):

sudo ./bin/meu_logger ping -c 4 8.8.8.8

4. Analisar os Resultados
Para visualizar o log sendo gerado em tempo real, abra um segundo terminal e utilize o comando tail:

tail -f syscall_log.txt

🎥 Vídeo de Demonstração
Um vídeo completo demonstrando o funcionamento da ferramenta, o processo de compilação e a análise da saída está disponível no YouTube. O vídeo cobre os principais casos de uso descritos no plano de testes.

https://www.youtube.com/watch?v=lzfYdBjs-o8&t=5s












# Logger de Chamadas de Sistema


PLANO DE TESTES - SYSCALL LOGGER
=================================


Este documento descreve os passos para testar a ferramenta de rastreamento de chamadas de sistema.

--- CONFIGURACAO DO AMBIENTE DE TESTE ---

Para executar os testes de forma eficiente, utilize duas janelas de terminal lado a lado dentro da VM Ubuntu.

1. TERMINAL 1 (O Logger):
   - Usado para executar o programa logger.
   - Navegue até a pasta raiz do projeto. Ex: cd ~/Documentos/syscall-logger-projeto/

2. TERMINAL 2 (O Observador):
   - Usado para monitorar o arquivo de log em tempo real.
   - Execute o comando e deixe-o rodando:
     $ tail -f syscall_log.txt


--- TESTE 1: COMANDO SIMPLES (ls) ---

Objetivo: Testar a funcionalidade básica de rastreamento e o ciclo de vida de um processo simples.

COMANDO A EXECUTAR (no Terminal 1):
$ ./bin/meu_logger ls -l /etc

O QUE PROCURAR NO LOG (no Terminal 2):
- execve: O início da execução do /bin/ls.
- brk, mmap, mprotect: Chamadas de gerenciamento de memória.
- openat: Abertura de arquivos de configuração e do diretório /etc.
- getdents64: Leitura do conteúdo do diretório.
- newfstatat / stat: Leitura dos metadados (permissões, tamanho) de cada arquivo.
- write: Escrita da saída (a lista de arquivos) no terminal.
- close: Fechamento dos arquivos.
- exit_group: O fim do programa.


--- TESTE 2: ATIVIDADE DE REDE (ping) ---

Objetivo: Observar syscalls relacionadas a operações de rede.

COMANDO A EXECUTAR (no Terminal 1):
$ sudo ./bin/meu_logger ping -c 4 8.8.8.8
* Nota: 'sudo' pode ser necessário para criar sockets de rede.

O QUE PROCURAR NO LOG (no Terminal 2):
- socket: Criação do ponto de comunicação de rede.
- connect / sendto: Envio dos pacotes de ping.
- recvfrom / poll: Espera pelas respostas dos pings.
- nanosleep: Pausa de 1 segundo entre cada ping.
- close: Fechamento do socket.
* Dica: Rodar sem '-c 4' é um bom teste para o encerramento com Ctrl+C.


--- TESTE 3: LEITURA DE ARQUIVO (cat) ---

Objetivo: Rastrear o ciclo completo de uma operação de I/O de arquivo.

COMANDOS A EXECUTAR (no Terminal 1):
1. Crie o arquivo de teste:
   $ echo "Ola, mundo do ptrace!" > teste.txt

2. Execute o logger no comando 'cat':
   $ ./bin/meu_logger cat teste.txt

O QUE PROCURAR NO LOG (no Terminal 2):
- openat: Abertura do arquivo "teste.txt". O argumento será um ponteiro para essa string.
- read: Leitura do conteúdo do arquivo.
- write: Escrita do conteúdo lido para a saída padrão (o terminal).
- close: Fechamento do arquivo.


--- TESTE AVANÇADO: RASTREAMENTO DE PROCESSOS FILHOS (fork) ---

Objetivo: Verificar se o logger consegue detectar a criação de novos processos.

MODIFICACAO NECESSARIA NO CODIGO (src/main.c):
- Altere a linha do ptrace SETOPTIONS para incluir o rastreamento de forks:
  ptrace(PTRACE_SETOPTIONS, child_pid, 0, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE);
- Recompile com: make

COMANDO A EXECUTAR (no Terminal 1):
$ ./bin/meu_logger sh -c "echo 'Iniciando...' && ls -l"

O QUE PROCURAR NO LOG (no Terminal 2):
- clone / fork / vfork: As syscalls que indicam a criação de um novo processo.
- execve: A execução do comando 'ls' pelo processo filho.
