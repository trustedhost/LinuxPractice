#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <sys/socket.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    int ret, sock_fd[2];
    int status;
    char buf[] = "Hello World!", line[BUFSIZ];
    pid_t pid;

    ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, sock_fd); /* generate socket pair */
    if (ret == -1) {
        perror("socketpair()");
        return -1;
    }

    printf("socket 1: %d\n", sock_fd[0]);
    printf("socket 2: %d\n", sock_fd[1]);

    if((pid = fork()) < 0) {
        perror("fork()");
    } else if (pid == 0) { /* child process */
        write(sock_fd[0], buf, strlen(buf) + 1); /* send data to parent process */
        printf("Data send: %s\n", buf);

        close(sock_fd[0]);

    } else {
        wait(&status);
        read(sock_fd[1], line, BUFSIZ);
        printf("Received Data : %s\n", line);

        
        printf("%d\n", status);
        printf("%d\n", WEXITSTATUS(status)); // 하위 8비트만 표시하도록 함. 
        close(sock_fd[1]);
    }
    exit(10);
}