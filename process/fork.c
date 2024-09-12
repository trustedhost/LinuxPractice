#include <stdio.h>
#include <unistd.h>

static int g_var = 1;  /* data */
char str[] = "PID";

int main(int argc, char **argv) {
    int var; /* stack */
    pid_t pid;
    var = 92;

    if ((pid = fork()) < 0) {
        perror("[ERROR] : fork() \n");
    } else if (pid == 0) { /* child process */
        printf("child process start------------------------------------------------\n");
        printf("Parent %s from Child Process (%d) : %d \n", str, getpid(), getppid());
        printf("address of variable (var): %p \n", &var);
        printf("address of variable (g_var): %p \n", &g_var);
        g_var++;
        var++;
        printf("------------------------------------------------\n");
        printf("address of variable (var): %p \n", &var);
        printf("address of variable (g_var): %p \n", &g_var);
        printf("child process end------------------------------------------------\n");
    } else { /* parent process */
        printf("parent process start------------------------------------------------\n");
        printf("Child %s from Parent Process (%d) : %d \n", str, getpid(), pid);
        printf("address of variable (var): %p \n", &var);
        printf("address of variable (g_var): %p \n", &g_var);
        sleep(1);
        printf("parent process end------------------------------------------------\n");
    }
    /* print variables */
    printf("pid = %d, Global var = %d, var = %d\n", getpid(), g_var, var);
    printf("address of variable (var): %p \n", &var);
    printf("address of variable (g_var): %p \n", &g_var);
    return 0;
}