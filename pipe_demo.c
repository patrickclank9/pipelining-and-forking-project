#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

#define MAX_ARGS 20

void syserror(const char *);

int main(void)
{
    int pfd[2];
    pid_t pid;
    const char *args[MAX_ARGS];
    const char *args2[MAX_ARGS];

    if (pipe(pfd) == -1)
        syserror("Could not create a pipe");
    switch (pid = fork())
    {
    case -1:
        syserror("Fork failed.");
        break;
    case 0:
        if (close(0) == -1)
            syserror("Could not close stdin I");
        dup(pfd[0]);
        if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
            syserror("Could not close pfds I");
        args[0] = "wc";
        args[1] = 0;
        execvp(args[0], (char **)args);
        syserror("unsuccessfull execlp I!!");
    }
    fprintf(stderr, "The first child's pid is: %d\n", pid);
    switch (pid = fork())
    {
    case -1:
        syserror("Could not fork successfully II");
        break;
    case 0:
        if (close(1) == -1)
            syserror("Could not close stdout");
        dup(pfd[1]);
        if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
            syserror("Could not close pfds II");
        args2[0] = "cat";
        args2[1] = "input.txt";
        args2[2] = 0;
        execvp(args[0], (char **)args);
        syserror("Execlp error");
    }
    fprintf(stderr, "The second child's pid is: %d\n", pid);
    if (close(pfd[0]) == -1 || close(pfd[1]) == -1)
        syserror("parent could not close file descriptors!!");
    while ((pid = wait((int *)0)) != -1)
        ; /* fprintf(stderr,"%d\n", pid) */
}

void syserror(const char *s)
{
    extern int errno;

    fprintf(stderr, "%s\n", s);
    fprintf(stderr, " (%s)\n", strerror(errno));
    exit(1);
}
