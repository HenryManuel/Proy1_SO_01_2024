#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <regex.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 50
#define MAX_LINE_LENGTH 100

bool validar_comando(const char *cmd, char **programa, char *argv[]) {
    char *comando = strdup(cmd); //Hago una copia de lo que recibi por el parametro cmd (Programa y argumentos)
    *programa = strtok(comando, " \n"); //Tambien se guarda el programa en la variable programa para tener el nombre del programa en el main
    argv[0] = *programa; //Guarda en la primera posicion de argv el programa
   
    //Valido que el comando debe tener SIEMPRE por lo menos el programa
    if (programa == NULL){
        free(comando); //Libero la memoria que fue asignada al crear la copia comando
        return false; //Indico que el formato del comando es invalido
    }

    int count = 1;
    
    //Se queda con la linea sin el programa, es decir los argumentos
    char *argumentos = strtok(NULL, "");  //El NULL indica que debe comenzar a partir de donde se quedo la ultima tokenizacion

    //Separo el primer argumento de argumentos para utilizarlo en el while de abajo (Sin espacios y sin tabuladores)
    char *argumento = strtok(argumentos, " \t");

    while (argumento != NULL) {
        argv[count]=strdup(argumento); //Anexando el argumento al argv
        count++; //Incrementa contador de argumentos
        argumento = strtok(NULL, " \t"); //Salto a la siguiente palabra de argumentos partiendo de la ultima tokenizacion realizada (Fue en argumentos)
    }

    return true; //

}

bool validar_linea(const char *ln, char *programa1, char *argv1[], char *programa2, char *argv2[], char **operador) {
    char *linea = strdup(ln); //Realizo una copia de la linea recibida por parametro en la variable linea
    char *palabra = strtok(linea, " \t\n"); //Separo por espacios y tabuladores la primera palabra de la linea

    //La idea es recorrer la linea y lograr diferenciar comando1 y comando2
    char comando1[MAX_LINE_LENGTH] = {'\0'};
    char comando2[MAX_LINE_LENGTH] = {'\0'};
    bool flagOperator = false; //Flag para validar cuando termina un comando y comienza el otro.    Falso: No he encontrado operador, estoy construyendo Comando 1.
                                                                                                    //Verdad: Operador Encontrado, estoy construyendo Comando 2
    bool validacion_comando = false;

    while (palabra != NULL) {
        
        //La palabra que se esta recorriendo es el operador
        if((strcmp(palabra,"|") == 0) || (strcmp(palabra,"||") == 0) || (strcmp(palabra,"&&") == 0)) {
            flagOperator = true; //Marcando operador conseguido
            *operador = strdup(palabra); //Guardando Operador para utilizarlo en el main
            palabra = strtok(NULL, " \t\n"); //Pasa a la siguiente palabra
            continue;
        }

        if(flagOperator){
            
            //Agrego palabra a palabra a comando2
            if(strcmp(comando2,"\0") != 0){
                strcat(comando2, " ");
            }
            strcat(comando2, palabra);
        }else{

            //Agrego palabra a palabra a comando1
            if(strcmp(comando1,"\0") != 0){
                strcat(comando1, " ");
            }
            strcat(comando1, palabra);
        }

        //Pasa a la siguiente palabra
        palabra = strtok(NULL, " \t\n");
    }
    
    //Comando 1 y 2 no son vacios
    if(strcmp(comando1,"\0") != 0 && strcmp(comando2,"\0") != 0){
        //Valido ambos comandos
        validacion_comando = validar_comando(comando1, programa1, argv1) && validar_comando(comando2, programa2, argv2);
    }else{

        //Primero comando siempre tiene algo y esta seguido de un operador
        if(flagOperator){
            free(linea);
            return false;
        }else{
            //Caso de comando solo
            validacion_comando = validar_comando(comando1, programa1, argv1);
        }
    }
    free(linea); //Libero espacio de memoria asignado a la copia realizada en la variable linea
    return validacion_comando;
}

void executer(char *programaMain, char *argvMain[]) {
    char programaPath[100];
    sprintf(programaPath, "/bin/%s", programaMain);

    pid_t pid = fork(); //Crea un proceso hijo
    //printf("PID %d\n", pid);
    if (pid == -1) { //Error al crear el proceso
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { 
        fflush(stdout);
        // Child process
        //Realizo la ejecucion del comando enviado por parametros 
        if (execvp(programaPath, argvMain) == -1) {
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    } else {
        // Parent process
        int status;
        //Espero que culmine la ejecucion del proceso hijo
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

         if (!WIFEXITED(status)) {
            printf("Child process exited abnormally\n");
         }
    }
}

int main() {
    char linea[MAX_LINE_LENGTH];
    char *programaMain1 = NULL;
    char *argvMain1[MAX_ARGS];
    char *programaMain2 = NULL;
    char *argvMain2[MAX_ARGS];
    char *operador = NULL;

    while (1){

        for(int i = 0; i <= MAX_ARGS-1; i++) {
            argvMain1[i]=NULL;
            argvMain2[i]=NULL;
        }

        operador = NULL;
        programaMain1 = NULL;
        programaMain2 = NULL;

        printf("Ingrese una linea de comando: ");
        fgets(linea, sizeof(linea), stdin);

        if (strcmp(linea, "salir\n") == 0){
            printf("Adios\n");
            break;
        } else {
            if (validar_linea(linea, &programaMain1, argvMain1, &programaMain2, argvMain2, &operador)) {
                if (operador == NULL || (strcmp(operador, "&&") == 0 || strcmp(operador, "||") == 0)) {
                    char programaPath[100];
                    sprintf(programaPath, "/bin/%s", programaMain1);

                    pid_t pid = fork();
                    // printf("PID %d\n", pid);
                    if (pid == -1) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    } else if (pid == 0) {
                        fflush(stdout);
                        // Child process
                        if (execvp(programaPath, argvMain1) == -1) {
                            perror("execvp");
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        // Parent process
                        int status;
                        if (waitpid(pid, &status, 0) == -1) {
                            perror("waitpid");
                            exit(EXIT_FAILURE);
                        }

                        if (WIFEXITED(status)) {
                            if (operador != NULL) {
                                if ( (!WEXITSTATUS(status) && strcmp(operador,"&&") == 0) || //Hay un operador && y el primer comando se ejecuto correctamente, devolvio status 0
                                (WEXITSTATUS(status) && strcmp(operador,"||") == 0) ) { //El operador es un || y el primer comando dio error, devolvio status 1
                                    executer(programaMain2,argvMain2);
                                }
                            }
                        } else {
                            printf("Child process exited abnormally\n");
                        }
                    }
                }else if (operador != NULL && strcmp(operador, "|") == 0){
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
                        // Child process 1
                        close(pipefd[0]); // Close unused read end
                        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end of pipe
                        close(pipefd[1]); // Close write end of pipe
                        executer(programaMain1, argvMain1); // Execute the first command
                        exit(EXIT_SUCCESS);
                    }

                    pid_t pid2 = fork();
                    if (pid2 == -1) {
                        perror("fork");
                        exit(EXIT_FAILURE);
                    } else if (pid2 == 0) {
                        // Child process 2
                        close(pipefd[1]); // Close unused write end
                        dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to read end of pipe
                        close(pipefd[0]); // Close read end of pipe
                        executer(programaMain2, argvMain2); // Execute the second command
                        exit(EXIT_SUCCESS);
                    }

                    // Close both ends of the pipe in the parent process
                    close(pipefd[0]);
                    close(pipefd[1]);

                    // Wait for both child processes to complete
                    waitpid(pid1, NULL, 0);
                    waitpid(pid2, NULL, 0);
                    
                }
                        
            } else {
                printf("Formato invalido.\n");
            }
        }

    }
    return 0;
}

/*While true hasta comando salir, ctrl c o EOF*/

/*Los errores los paso por fprintf stderr */