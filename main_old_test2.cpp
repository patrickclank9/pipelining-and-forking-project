/**
 *  By: Patrick Hernandez
 *  I will change this into the format we discussed, but this is the code I already had before.
 *  And it all works so I'm turning it in
 */

#include<iostream>

#include<stdio.h> 
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>

#define MAX_LINE 1024
#define MAX_ARGS 20
#define MAX_INPUT_CHAR_COUNT 200
#define DEBUG false

bool charOfIntrest(int _c); //similar to isSpace()
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
    bool bHasInputRedirect, bHasOutputRedirect, bHasPipe;
    bHasInputRedirect = bHasOutputRedirect = bHasPipe = false;
    int numberOfProgramsToPipe = 0;

    char line[ MAX_LINE ];
    char *argsForExec[ MAX_ARGS ];
    int llen;

    std::cin.getline( line, MAX_LINE, '\n' );
    llen = strlen( line );

    // skip leading spaces.

    char *start = line;
    int argsForExecCount = 0;
    int endIdx = -1; 
    int startIdx = 0;
    for( startIdx = endIdx + 1; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx ) 
        ;
    if( line[startIdx] == '\0' ) {
            std::cout << "Exiting...\n";
            return 0;
    }
    
    for(int i = 0; line[startIdx] != '\0'; i++){
        for( startIdx = endIdx + 1; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx ) 
            ;

        for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ) && ! charOfIntrest(line[endIdx]); ++endIdx ) 
            ;
        line[endIdx] = '\0';
        argsForExec[i] = line + startIdx;
        ++argsForExecCount;
    }
    
    // for( startIdx = endIdx + 1; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx ) 
    //     ;

    // if( line[startIdx] == '\0' ) {
    //     std::cout << "Empty input line.\n";
    //     return 0;
    // }
    // for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ); ++endIdx ) 
    //     ;
    // line[endIdx] = '\0';
    // argsForExec[0] = line + startIdx;


    // for( startIdx = endIdx + 1; line[startIdx] != '\0' && isspace( line[startIdx] ); ++startIdx ) 
    //     ;

    // if( line[startIdx] == '\0' ) {
    //     std::cout << "Empty input line.\n";
    //     return 0;
    // }
    // for( endIdx = startIdx + 1;  line[endIdx] != '\0' && ! isspace( line[endIdx] ); ++endIdx ) 
    //     ;
    // line[endIdx] = '\0';
    // argsForExec[1] = line + startIdx;
    
    for(int i = 0; i < argsForExecCount; ++i)
        if(*argsForExec[i] != '\0')
            std::cout << "Word is: --" << argsForExec[i] << "--\n";
        else
            std::cout << "Word is: NULL" << "\n";


    return 0;
}

bool charOfIntrest(int _c){
    switch(_c){
        case '<':
        case '>':
        case '|':
        case '"':
            return true;
        default:
            return false;
    }
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
    dup2(stdout_cpy, 0);
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