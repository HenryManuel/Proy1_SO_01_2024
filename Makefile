#
# Minishell
#

# Regla por defecto: compilar el proyecto completo
all: minishell

# Regla para limpiar los archivos generados por la compilación
clean:
	rm -rf minishell.o minishell

# Regla para compilar el ejecutable
minishell: minishell.o
	gcc -o minishell minishell.o

# Regla para compilar el archivo minishell.c en minishell.o
minishell.o: minishell.c
	gcc -c minishell.c -o minishell.o
