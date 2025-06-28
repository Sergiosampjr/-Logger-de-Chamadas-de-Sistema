# Como usar:
# Para compilar o projeto inteiro, basta digitar no terminal: make
# Para apagar todos os arquivos compilados e limpar o projeto, digite: make clean

# Nome do compilador que vamos usar
CC = gcc

# Flags de compilação: -g para incluir informações de debug
# -Wall para mostrar todos os avisos (muito útil!)
CFLAGS = -g -Wall

# Onde procurar por arquivos de cabeçalho (.h)
INCLUDES = -I./src

# Onde o executável final vai ficar e qual o seu nome
TARGET = bin/meu_logger

# Lista de arquivos fonte (.c)
SOURCES = src/main.c src/parser.c

# Converte a lista de fontes .c para arquivos objeto .o
OBJECTS = $(SOURCES:.c=.o)

# A "receita" principal. É executada quando você digita 'make'
all: $(TARGET)

# Receita para criar o executável final a partir dos arquivos objeto
$(TARGET): $(OBJECTS)
	@mkdir -p bin  # Garante que a pasta bin/ exista
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Executável [$(TARGET)] criado com sucesso!"

# Receita genérica para criar arquivos .o a partir de arquivos .c
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Receita para limpar os arquivos gerados (compilados)
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Arquivos compilados foram removidos."

.PHONY: all clean
