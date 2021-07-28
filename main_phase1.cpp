/**
 *  By: Patrick Hernandez
 *  I will change this into the format we discussed, but this is the code I already had before.
 *  And it all works so I'm turning it in
 */

#include<stdio.h> 
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>

#define MAX_ARGS 20
#define MAX_INPUT_CHAR_COUNT 200
#define DEBUG false

pid_t forkAndExecute(char *argv[]);
int openFileToInput(char *inputFileName);
int openFileToOutput(char *outputFileName);
void redirectInputAndExecute(char *programArgs[], char *inputFileName);
void redirectOutputAndExecute(char *programArgs[], char *outputFileName);
int setInputToPipeInput(int pfd[]);
int setOutputToPipeOutput(int pfd[]);
void closePipe(int pfd[]);
pid_t forkAndExecuteWithPipeInput(char *argv[], int pfd[]);
pid_t forkAndExecuteWithPipeOutput(char *argv[], int pfd[]);
void syserror(const char *);

int main(){
    //get input
    char inputStr[MAX_INPUT_CHAR_COUNT];
    fprintf(stdout, "> ");
    fgets(inputStr, MAX_INPUT_CHAR_COUNT, stdin);
    inputStr[strcspn(inputStr, "\n")] = 0; //removes '\n' from input

    //parse into array
    char *inputArr[MAX_ARGS];
    int inputArrCount = 0;
    inputArr[inputArrCount] = strtok(inputStr, " ");
    while(inputArr[inputArrCount] != NULL)
        inputArr[++inputArrCount] = strtok(NULL, " ");


    /** Later, parse the entire command into an array of argv[]
     *  Keep track of the number of '|'s
     *  Have a flag of wether '<' or '>' was used, if so 
     * '<' will only apply to the first commend and 
     * '>' only to the last command.
     */ 

    char *argv[MAX_ARGS];
    int argc = 0;
    int stdin_cpy = 0;
    int stdout_cpy = 1;
    for (int i = 0; i < inputArrCount; ++i){ //USE WHILE LOOP INSTEAD
        if(*inputArr[i] == '|' || *inputArr[i] == '<' || *inputArr[i] == '>') {
            if(DEBUG){
                printf("argc = %d ... argv:\n", argc);
                printf("\t%s", argv[0]);
                for (int j = 1; j < argc; ++j) printf("\t%s", argv[j]);
                printf("\n---------------------\n");
            }

            switch (*inputArr[i])
            {
                case '<':
                    if(!inputArr[++i])
                        syserror("No in file specified");
                        stdin_cpy = openFileToInput(inputArr[i]);
                    break;
                
                case '>':
                    if(!inputArr[++i])
                        syserror("No out file specified");
                        stdout_cpy = openFileToOutput(inputArr[i]);
                    break;

                case '|':
                    char *argv2[MAX_ARGS];
                    int argc2 = 0;
                    if(!inputArr[++i])
                        syserror("No command to pipe to specified");
                    while(i < inputArrCount)
                        argv2[argc2++] = strdup(inputArr[i++]);
                    argv2[argc2] = 0;
                    
                    int pfd[2];
                    if (pipe(pfd) == -1)
                        syserror("Could not create a pipe");

                    if(DEBUG){
                        printf("argc2 = %d ... argv2:\n", argc2);
                        printf("\t%s", argv2[0]);
                        for (int j = 1; j < argc2; ++j) printf("\t%s", argv2[j]);
                        printf("\n---------------------\n");
                    }

                    pid_t pid = forkAndExecuteWithPipeInput(argv2, pfd);
                    pid = forkAndExecuteWithPipeOutput(argv, pfd);
                    closePipe(pfd);
                    while ((pid = wait((int *)0)) != -1)
                        ; 

                    return 0;
                    break;
            }
        } else {
            argv[i] = strdup(inputArr[i]);
            argv[++argc] = 0;
        }
    }

    if(DEBUG){
        printf("argc = %d ... argv:\n", argc);
        printf("\t%s", argv[0]);
        for (int j = 1; j < argc; ++j) printf("\t%s", argv[j]);
        printf("\n---------------------\n");
    }
    
    
    pid_t pid = forkAndExecute(argv);
    while ((pid = wait((int *)0)) != -1);

    dup2(stdin_cpy, 0);
    dup2(stdout_cpy, 1);


    return 0;
}

pid_t forkAndExecute(char *argv[]){
    pid_t pid;
    switch (pid = fork())
    {
    case -1:
        syserror("Fork failed.");
        break;
    case 0:
        execvp(argv[0], (char **)argv);
        syserror("unsuccessfull execvp");
    }
    return pid;
}

int openFileToInput(char *inputFileName){
    // Input: char* to file name
    // Output: a copy of stdin's file descriptor
    int inputfd;
    if ((inputfd = open(inputFileName, O_RDONLY)) < 0)
        syserror("input file does not exist.");
    int stdin_cpy = dup(0);
    if (close(0) == -1)
        syserror("Could not close stdin");
    dup(inputfd);
    return stdin_cpy;
}

int openFileToOutput(char *outputFileName){
    int outputfd;
    if ((outputfd = open(outputFileName, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR)) < 0)
        syserror("output file did not exist and failed to create.");
    int stdout_cpy = dup(0);
    if(close(1) == -1)
        syserror("Could not close stdout");
    dup(outputfd);
    return stdout_cpy;
}

void redirectInputAndExecute(char *programArgs[], char *inputFileName){
    int stdin_cpy = openFileToInput(inputFileName);
    forkAndExecute(programArgs);
    dup2(stdin_cpy, 0);
}

void redirectOutputAndExecute(char *programArgs[], char *outputFileName){
    int stdout_cpy = openFileToOutput(outputFileName);
    forkAndExecute(programArgs);
    dup2(stdout_cpy, 1);
}

void closePipe(int pfd[]){
    if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
        syserror("Could not close pfds");
}

int setInputToPipeInput(int pfd[]){
    // Input: int to pipe array of fds
    // Output: a copy of stdin's file descriptor
    int stdin_cpy = dup(0);
    if (close(0) == -1)
        syserror("Could not close stdin");
    dup(pfd[0]);
    closePipe(pfd);
    return stdin_cpy;
}

int setOutputToPipeOutput(int pfd[]){
    // Input: int to pipe array of fds
    // Output: a copy of stdout's file descriptor
    int stdout_cpy = dup(0);
    if (close(1) == -1)
        syserror("Could not close stdout");
    dup(pfd[1]);
    closePipe(pfd);
    return stdout_cpy;
}

pid_t forkAndExecuteWithPipeInput(char *argv[], int pfd[]){
    pid_t pid;
    switch (pid = fork()){
        case -1:
            syserror("Fork failed.");
            break;
        case 0:
            setInputToPipeInput(pfd);
            execvp(argv[0], (char **)argv);
            syserror("unsuccessfull execlp!!");
    }
    return pid;
}

pid_t forkAndExecuteWithPipeOutput(char *argv[], int pfd[]){
    pid_t pid;
    switch (pid = fork()){
        case -1:
            syserror("Could not fork successfully");
            break;
        case 0:
            setOutputToPipeOutput(pfd);
            execvp(argv[0], (char **)argv);
            syserror("unsuccessfull execvp");
    }
    return pid;
}

void syserror(const char *s){
    extern int errno;
    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}