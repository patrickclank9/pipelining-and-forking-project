/**
 *  By: Patrick Hernandez
 *  I have implemented the use of <, >, and multiple |'s
 *  There are 2 catches, you must have at least 1 space on both sides of the <, >, and | symbols.
 *  And any commands using quotations (") will not group into a single arguement.
 *  These are problems with the parser. However, if it were to parse correctly, my fork and piping 
 *  code would work with it.
 * 
 *  The program terminates when the promt is left empty or end-of-file is inputed (ctrl-d)
 *  
 *  for testing I mostly used w, wc, and cat in ways such as:
 *  w
 *  w | wc
 *  cat < input.txt > output.txt
 *  cat < input.txt | wc > output.txt
 *  w | wc | wc | wc | wc > output.txt
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
#define MAX_ARGS 40
#define DEBUG false
#define RUN_IN_LOOP true

struct Pipe
{
    int pfd[2];
};

bool charOfIntrest(int _c); //similar to isSpace()
void printNullTerminatedArr(char *argv[]);
pid_t forkAndExecute(char *argv[]);
int openFileToInput(char *inputFileName);
int openFileToOutput(char *outputFileName);
int setInputToPipeInput(int pfd[]);
int setOutputToPipeOutput(int pfd[]);
void closePipe(int pfd[]);
void closePipesRange(Pipe pipes[], int from, int to);
pid_t forkAndExecuteWithPipeInput(char *argv[], int pfd[]);
pid_t forkAndExecuteWithPipeOutput(char *argv[], int pfd[]);
void syserror(const char *);

int main(){
do{
    bool bHasInputRedirect, bHasOutputRedirect, bHasPipe;
    bHasInputRedirect = bHasOutputRedirect = bHasPipe = false;

    char *inputFile, *outputFile;

    char line[ MAX_LINE ];
    char *argsForExec[ MAX_ARGS ];
    int llen;

    for(int i = 0; i < MAX_LINE; ++i)
        line[i] = '\0';

    fprintf(stdout, "> ");
    std::cin.getline( line, MAX_LINE, '\n' );
    llen = strlen( line );

    for(int i = 0; i < llen; ++i){
        if(line[i] == '<'){
                bHasInputRedirect = true;
            } else if(line[i] == '>'){
                bHasOutputRedirect = true;
            } else if(line[i] == '|'){
                bHasPipe = true;
            }
    }

    // skip leading spaces.

    char *start = line;
    int argsForExecCount = 0;
    int endIdx = -1; 
    int startIdx = 0;

    for(int i = 0; i < MAX_ARGS; ++i)
        argsForExec[i] = nullptr;

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

    for(int i = 0; i < argsForExecCount; ++i){
        if(charOfIntrest(*argsForExec[i]))
            argsForExec[i] = nullptr;
    }
    
    if(DEBUG){
        std::cout << "argsForExecCount =\t" << argsForExecCount << "\n";
        for(int i = 0; i < argsForExecCount; ++i)
            if(argsForExec[i] != nullptr)
                std::cout << "argsForExec[" << i << "] =\t'" << argsForExec[i] << "'\n";
            else
                std::cout << "argsForExec[" << i << "] =\tNULL" << "\n";
        bHasInputRedirect ? std::cout << "bHasInputRedirect:\ttrue\n" : std::cout << "bHasInputRedirect:\tfalse\n";
        bHasOutputRedirect ? std::cout << "bHasOutputRedirect:\ttrue\n" : std::cout << "bHasOutputRedirect:\tfalse\n";
        bHasPipe ? std::cout << "bHasPipe:\t\ttrue\n" : std::cout << "bHasPipe:\t\tfalse\n";   
    }
    
    char **argv[MAX_ARGS];
    int argc = 0;
    for(int i = 0; i < MAX_ARGS; i++)
        argv[i] = nullptr;

    int index = 0;        
    argv[argc] = &argsForExec[index];
    ++argc;

    while(argsForExec[index])
        index++; //go until index is at a Nullptr
    index++; //go one more to the next element

    if(bHasInputRedirect){
        //openFileToInput(argsForExec[2]);
        inputFile = argsForExec[index];
        DEBUG ? std::cout << "InputFile:\t\t'" << inputFile << "'\n" : std::cout;
        index++;
        while(argsForExec[index])
            index++; //go until index is at a Nullptr
        index++;
    }

    if(bHasOutputRedirect){
        outputFile = argsForExec[argsForExecCount - 2]; // [... , outfile, NULL]
        DEBUG ? std::cout << "Outputfile:\t\t'" << outputFile << "'\n" : std::cout;
        argsForExecCount -= 2;
    }

    while(index < argsForExecCount){
        argv[argc] = &argsForExec[index];
        argc++;
        while(argsForExec[index])
            index++; //go until index is at a Nullptr
        index++; //go one more to the next element
    }

    int numPipes = argc - 1;
    DEBUG ? std::cout << "numPipes =\t\t" << numPipes << std::endl : std::cout;

    if(DEBUG){
        for(int i = 0; i < argc; ++i){
            std::cout << "argv[" << i << "] = \t\t";
            printNullTerminatedArr(argv[i]);
            std::cout << std::endl;
        }
    }
    
    /** at this point, the input is parsed. variables to use:
     *  argv[i] where i is the arg array to use
     *  argc which is the number of elements in argv
     *  bHasInputRedirect, bHasOutputRedirect, bHasPipe
     *  numPipes
     */
    pid_t pid;

    int stdin_cpy = 0;
    int stdout_cpy = 1;

    if(!bHasPipe){
        if(bHasInputRedirect){
            stdin_cpy = openFileToInput(inputFile);
        }
        if(bHasOutputRedirect){
            stdout_cpy = openFileToOutput(outputFile);
        }
        pid = forkAndExecute(argv[0]);
        dup2(stdin_cpy, 0);
        dup2(stdout_cpy, 1);
    }

    if(bHasPipe){
        Pipe pipes[MAX_ARGS];
        for(int i = 0; i < numPipes; ++i){
            if (pipe(pipes[i].pfd) == -1)
                syserror("Could not create a pipe");
        }
        pid_t pid;

        if(bHasInputRedirect){
            stdin_cpy = openFileToInput(inputFile);
        }

        switch (pid = fork()){ //take care of first command
            case -1:
                syserror("Could not fork successfully");
                break;
            case 0:
                setOutputToPipeOutput(pipes[0].pfd);
                closePipesRange(pipes, 0, numPipes - 1);
                execvp(argv[0][0], (char **)argv[0]);
                syserror("unsuccessfull execvp");
        }

        int idx = 1; // start on the second command
        while(idx < argc-1){ //take care of middle cases if there are any
            switch (pid = fork()){
                case -1:
                    syserror("Could not fork successfully");
                    break;
                case 0:
                    setInputToPipeInput(pipes[idx-1].pfd);
                    setOutputToPipeOutput(pipes[idx].pfd);
                    closePipesRange(pipes, 0, numPipes - 1);
                    execvp(argv[idx][0], (char **)argv[idx]);
                    syserror("unsuccessfull execvp");
            }
            idx++;
        }

        if(bHasOutputRedirect){
            stdout_cpy = openFileToOutput(outputFile);
        }

        switch (pid = fork()){ // take care of last case
            case -1:
                syserror("Could not fork successfully");
                break;
            case 0:
                setInputToPipeInput(pipes[idx-1].pfd);
                closePipesRange(pipes, 0, numPipes - 1);
                execvp(argv[idx][0], (char **)argv[idx]);
                syserror("unsuccessfull execvp");
        }
        closePipesRange(pipes, 0, numPipes - 1);
        dup2(stdin_cpy, 0);
        dup2(stdout_cpy, 1);
    }
    while ( wait( NULL ) != -1 ); //wait for all children to terminate
    dup2(stdin_cpy, 0);
    dup2(stdout_cpy, 1);
}while(RUN_IN_LOOP);
return 0;
}

void printNullTerminatedArr(char *argv[]){
    std::cout << "[";
    if(argv[0])
        std::cout << argv[0];
    for(int i = 1; argv[i]; ++i){
        std::cout << ", " << argv[i];
    }
    std::cout << "]";
}

bool charOfIntrest(int _c){
    switch(_c){
        case '<':
        case '>':
        case '|':
        case '\0':
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
    int stdout_cpy = dup(1);
    if(close(1) == -1)
        syserror("Could not close stdout");
    dup(outputfd);
    return stdout_cpy;
}

void closePipe(int pfd[]){
    if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
        syserror("Could not close pfds");
}

void closePipesRange(Pipe pipes[], int from, int to){
    for(int i = from; i <= to; ++i){
        closePipe(pipes[i].pfd);
    }
}

int setInputToPipeInput(int pfd[]){
    // Input: int to pipe array of fds
    // Output: a copy of stdin's file descriptor
    int stdin_cpy = dup(0);
    if (close(0) == -1)
        syserror("Could not close stdin");
    dup(pfd[0]);
    return stdin_cpy;
}

int setOutputToPipeOutput(int pfd[]){
    // Input: int to pipe array of fds
    // Output: a copy of stdout's file descriptor
    int stdout_cpy = dup(0);
    if (close(1) == -1)
        syserror("Could not close stdout");
    dup(pfd[1]);
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