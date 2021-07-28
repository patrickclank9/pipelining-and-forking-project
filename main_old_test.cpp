/**
 *  By: Patrick Hernandez
 *  I will change this into the format we discussed, but this is the code I already had before.
 *  And it all works so I'm turning it in
 */

//c libs
#include<stdio.h> 
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>

//cpp libs
#include<string>
#include<iostream>
#include <algorithm> 
#include <cctype>
#include <locale>

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


// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}


int main(){
    bool bHasInputRedirect, bHasOutputRedirect, bHasPipe;
    bHasInputRedirect = bHasOutputRedirect = bHasPipe = false;
    int numberOfProgramsToPipe = 0;

    //get input to std string then later convert to cstr

    std::string test = "    this is a test       ";
    std::cout << "Original: \"" << test << "\"" << std::endl;
    trim(test);
    std::cout << "Edited: \"" << test << "\"" << std::endl;
    printf("Cstringed: \"%s\"\n", test.c_str());



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