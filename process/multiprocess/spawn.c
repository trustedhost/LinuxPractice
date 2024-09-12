#include <stdio.h>
#include <sys/wait.h>
#include <spawn.h>

extern char **environ;

int system(char* cmd) {
    pid_t pid;
    int status;

    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attrs;    
    char *argv[] = {"/bin/sh", "-c", cmd, NULL};

    posix_spawn_file_actions_init(&actions);
    posix_spawnattr_init(&attrs);
    posix_spawnattr_setflags(&attrs, POSIX_SPAWN_SETSCHEDULER);

    posix_spawn(&pid, "/bin/sh", &actions, &attrs, argv, environ);
    // waitpid(pid, &status, 0);

    return status;
}

int main(int argc, char *argv[]) {
    while (*environ) {
        printf("%s\n", *environ++);
    }  
    system("ls -al");
    return 0;
}