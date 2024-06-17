#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 100
#define MAX_LINE_LENGTH 200

bool validar_comando(const char *cmd, char **programa, char *argv[]) {
	
	char *comando = strdup(cmd);
  	
    *programa = strtok(comando, " \n");
    argv[0] = *programa;
    
    if (programa == NULL){
    	free(comando);
        return false;
    }

    int count = 1;
    char *argumentos = strtok(NULL, "");  
    char *argumento = strtok(argumentos, " \t");
	
	while (argumento != NULL) {
    	argv[count] = strdup(argumento);
        count++;
        argumento = strtok(NULL, " \t");
    }
    
	return true;
}

bool validar_linea(const char *ln, char *programa1, char *argv1[], char *programa2, char *argv2[], char **operador) {

	char *linea = strdup(ln); 
    char *palabra = strtok(linea, " \t\n"); 
	char comando1[MAX_LINE_LENGTH] = {'\0'};
    char comando2[MAX_LINE_LENGTH] = {'\0'};
   	bool flagOperator = false;
    bool validacion_comando = false;
    
    while (palabra != NULL) {
        if((strcmp(palabra,"|") == 0) || 
        (strcmp(palabra,"||") == 0) || 
        (strcmp(palabra,"&&") == 0)) {
        flagOperator = true;
            *operador = strdup(palabra); 
            palabra = strtok(NULL, " \t\n");
            continue;
        }
        if(flagOperator){
            if(strcmp(comando2,"\0") != 0){
            	strcat(comando2, " ");
            }
            strcat(comando2, palabra);
        }else{
        	if(strcmp(comando1,"\0") != 0){
                strcat(comando1, " ");
            }
            strcat(comando1, palabra);
        }
		palabra = strtok(NULL, " \t\n");  
    }
    
    if(strcmp(comando1,"\0") != 0 && strcmp(comando2,"\0") != 0){
        validacion_comando = validar_comando(comando1, programa1, argv1) 
        && validar_comando(comando2, programa2, argv2);
    }else{
		if(flagOperator){
            free(linea);
            return false;
        }else{
            validacion_comando = validar_comando(comando1, programa1, argv1);
        }
    }
    
    free(linea);
    return validacion_comando;
}

void executer(char *programaMain, char *argvMain[]) {

    char programaPath[100];
    sprintf(programaPath, "/bin/%s", programaMain);
    pid_t pid = fork(); 
    
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
        fflush(stdout);
        if (execvp(programaPath, argvMain) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        
    } else {
        int status;
        
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
        
		if (!WIFEXITED(status)) {
            printf("Child process exited abnormally\n");
         }
    }
}

void childExecuter(char *programaMain, char *argvMain[]) {

    char programaPath[100];
    sprintf(programaPath, "/bin/%s", programaMain);
   	
    fflush(stdout);
    if (execvp(programaPath, argvMain) == -1) {
       perror("execvp");
       exit(EXIT_FAILURE);
    }
}

int main() {
    char linea[MAX_LINE_LENGTH]; 
    char *programaMain1 = NULL;
    char *argvMain1[MAX_ARGS];
    char *programaMain2 = NULL;
    char *argvMain2[MAX_ARGS];
    char *operador = NULL;
    char *EOFcheck;
	
	printf("Ingrese una linea de comando: ");
	
    while (fgets(linea, MAX_LINE_LENGTH, stdin) && strcmp(linea, "salir\n") != 0){
	
		for(int i = 0; i <= MAX_ARGS-1; i++) {
            argvMain1[i]=NULL;
            argvMain2[i]=NULL;
        }
		
		operador = NULL;
        programaMain1 = NULL;
        programaMain2 = NULL;
		
        if(strcmp(linea,"\n") == 0){
        	continue;
        }
        if (validar_linea(linea, &programaMain1, argvMain1, &programaMain2, argvMain2, &operador)) {
			if (operador == NULL || (strcmp(operador, "&&") == 0 || strcmp(operador, "||") == 0)) {
				char programaPath[100]; 
				sprintf(programaPath, "/bin/%s", programaMain1);	
				pid_t pid = fork();
				if (pid == -1) {
					exit(EXIT_FAILURE);
				} else if (pid == 0) {
					fflush(stdout);
					if (execvp(programaPath, argvMain1) == -1) {
						perror("execvp");
						exit(EXIT_FAILURE);
					}        
				} else {
					int status;
					if (waitpid(pid, &status, 0) == -1) {
						perror("waitpid");
						exit(EXIT_FAILURE);
					}
					if (WIFEXITED(status)) {
						if (operador != NULL) {
							if ((!WEXITSTATUS(status) && strcmp(operador,"&&") == 0) || 
                                (WEXITSTATUS(status) && strcmp(operador,"||") == 0)) {
                                    executer(programaMain2,argvMain2);
							}
						}   
					} else {
						printf("Child process exited abnormally\n");
					}
				}
			} else if (operador != NULL && strcmp(operador, "|") == 0){
                	
				int pipefd[2];
                    
				if (pipe(pipefd) == -1) {
					perror("pipe");
					exit(EXIT_FAILURE);
				}
					
				pid_t pid1 = fork();
                    
				if (pid1 == -1) {
					perror("fork");
					exit(EXIT_FAILURE);
				} else if (pid1 == 0) {
					close(pipefd[0]);
					dup2(pipefd[1], STDOUT_FILENO);
					close(pipefd[1]);
					childExecuter(programaMain1, argvMain1);
					exit(EXIT_SUCCESS);
				}
					
				pid_t pid2 = fork();
					
				if (pid2 == -1) {
					perror("fork");
					exit(EXIT_FAILURE);
				} else if (pid2 == 0) {
					close(pipefd[1]);
					dup2(pipefd[0], STDIN_FILENO); 
					close(pipefd[0]);
					childExecuter(programaMain2, argvMain2);
					exit(EXIT_SUCCESS);
				}
				close(pipefd[0]);
				close(pipefd[1]);
				waitpid(pid1, NULL, 0);
				waitpid(pid2, NULL, 0);
			}         
		} else {
			printf("Formato invalido.\n");
		}
		
		printf("Ingrese una linea de comando: ");
		fflush(stdout);
		
    }
    
    printf("Adiós!\n");
    
    return 0;
}

