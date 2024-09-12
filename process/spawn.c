#include <stdio.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

int system(char* cmd) {
    pid_t pid;
    int status;
    char *argv[] = {"/bin/sh", "-c", cmd, NULL};
    posix_spawn(&pid, "/bin/sh", NULL, NULL, argv, environ);
    waitpid(pid, &status, 0);

    return status;
}

int main(int argc, char *argv[]) {
    while (*environ) {
        printf("%s\n", *environ++);
    }  
    system("ls -al");
    return 0;
}