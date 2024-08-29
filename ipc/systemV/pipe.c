#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    pid_t pid;
    int pfd[2];
    char line[BUFSIZ]; /* BUFSIZ defined in stdio.h */
    int status;

    if (pipe(pfd) < 0) { /* generate pipe */
        perror("pipe()");
        return -1;
    }

    if ((pid = fork()) < 0) { /* generate process */
        perror("fork()");
        return -1;
    } else if (pid == 0) { /* process child process */
        close(pfd[0]);
        write(1, "Hello World\n", 12); /* by child to stdout */
        dup2(pfd[1], 1);
        write(1, "Hello World\n", 12); /* by child to pipe */
        execl("/bin/date", "date", NULL);
        close(pfd[1]);
        _exit(127);
    } else { /* process parent process */
        close(pfd[1]);
        while (read(pfd[0], line, BUFSIZ) > 0 ) {
            printf("Read from parent process [pid : %d]", getpid());
            printf("%s", line);
        }
        close(pfd[0]);
        waitpid(pid, &status, 0);
    }
    return 0;
}