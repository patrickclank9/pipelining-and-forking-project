#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<errno.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>

// The following is an example of the use of "fork" and "pipe" system
// calls. In addition, it demonstrate how to open a file using the "open" system-call.
// It runs
// sort -n inFile | uniq -c | sort -nr
// These commands sort a list of numbers found in "inFile" and print each number along
// with the number of itmes that it has been repeated in "inFile".
// To Compile it, assuming you call it "twoPipes.c":

// gcc twoPipes.c -o twoPipes.x
// ./twoPipes.x aFileThatContainsAListOfIntegersOnePerLine

void syserror( const char * );
void syserror( const char *s )
{
	fprintf(stderr, "%s", s);
        fprintf(stderr, " (%s)\n", strerror(errno) );
	exit( 1 );
}

int main( int argc, char *argv[] )
{
    int pfd[2];   
    int readFD;
    if ( argc != 2 ) {
        fprintf(stderr, "Usage: %s %s\n", argv[0], "nameOfATextFile" );
        exit( 1 );
    }
    if( (readFD = open( argv[1], O_RDONLY ) ) < 0 ) {  // Is the file readable?
        char *buf = (char *) malloc( strlen( argv[0] ) + strlen( argv[1] ) + 3 );
        sprintf( buf, "%s: %s", argv[0], argv[1] );
        syserror( buf );
    }
    close( readFD );

    if ( pipe(pfd) == -1 )
        syserror( "Failed to create a pipe" );
    pid_t firstChildPID;
    switch ( firstChildPID = fork() ) {
        case -1: syserror( "Failed to fork successfully" );

        case  0:  // The child process; will become "cat"
            if( (readFD = open( argv[1], O_RDONLY ) ) < 0 ) {  // read-only.
                char *buf = (char *) malloc( strlen( argv[1] ) + strlen("Failed to open ") + 1 );
                sprintf( buf, "Failed to open %s", argv[1] );
                syserror( buf );
            }

            if( dup2( readFD, 0 ) == -1 || dup2( pfd[1], 1 ) == -1 || close( pfd[0] ) == -1 ||
                close( pfd[1] ) == -1 || close( readFD ) )
                syserror( "Failed to run dup2 or to close the pipe file-descriptors in child 1." );

            execlp("sort", "sort", "-n", NULL);
            syserror( "execlp failed in the child (sort) process.  Terminating.");
    }
    // Parent process continues here....

    // Only the first child-process writes to pfd[1] and as such, it
    // is not needed by the parent or the next child-processes.
    close( pfd[1] );

    int pfd2[2]; // For communication btwn the second and the third children.
    pipe( pfd2 );
    switch ( fork() ) {
        case -1: // should kill the previous child-process...
            syserror( "Failed to fork successfully." );

        case  0: // to run wc
            if( dup2( pfd[0], 0 ) == -1 || close( pfd[0] ) == -1 ||  close( pfd2[0] ) == -1 || 
                dup2( pfd2[1], 1 ) == -1 || close( pfd2[1] ) == -1 )
                syserror( "Failed to run dup2 or to close the pipe file-descriptors in child 2" );
            
            execlp("uniq", "uniq", "-c", NULL);
            syserror( "execlp failed in the child (uniq) process.  Terminating.");
    }
    // the parent process...

    if( close( pfd[0] ) == -1 || close( pfd2[1] ) == -1 ) 
        syserror( "Failed to close pfd[0] or pfd2[1]. Terminating.");

    switch ( fork() ) {
        case -1: // should kill the previous child-processes...
            syserror( "Failed to fork successfully." );

        case  0: // to run wc
            if( dup2( pfd2[0], 0 ) == -1 || close( pfd2[0] ) == -1 )
                syserror( "Failed to run dup2 or to close the pipe file-descriptors in child 3." );

            execlp("sort", "sort", "-nr", NULL);
            syserror( "execlp failed in the child (sort) process.  Terminating.");
    }

    // the parent process...
    if ( close( pfd2[0] ) == -1 )
        syserror( "Not able to close pipes in the parent process." );
    while ( wait( NULL ) != -1 ) 
        ;
    return 0;
}
