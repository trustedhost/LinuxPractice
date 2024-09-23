#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int fd[2];
  if (pipe(fd) == -1) {
    perror("pipe");
    return 1;
  }

  int pid = fork();
  if (pid == -1) {
    perror("fork");
    return 1;
  }
  if (pid == 0) {
    char str[200];
    printf("Enter a string : ");
    fgets(str, sizeof(str), stdin);
    str[strlen(str) - 1] = '\0';
    close(fd[0]);
    write(fd[1], str, sizeof(str));
    close(fd[1]);
  } else {
    char str[20];
    close(fd[1]);
    if (read(fd[0], str, sizeof(str)) == -1) {
      perror("read");
      return 1;
    }
    close(fd[0]);
    printf("Received string : %s\n", str);
    wait(NULL);
  }
  return 0;
}