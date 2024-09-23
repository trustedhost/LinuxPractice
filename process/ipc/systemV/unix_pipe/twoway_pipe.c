#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int pipefd[2];  // C => P
  int pipefd2[2]; // P => C
  if (pipe(pipefd) == -1) {
    perror("pipe");
    return 1;
  }
  if (pipe(pipefd2) == -1) {
    perror("pipe");
    return 1;
  }

  int pid = fork();
  if (pid == -1) {
    perror("fork");
    return 1;
  }
  if (pid == 0) { // child process
    close(pipefd[0]);
    close(pipefd2[1]);
    int x;
    if (read(pipefd2[0], &x, sizeof(x)) == -1) {
      perror("read");
      return 1;
    }
    printf("Received x = %d\n", x);
    x *= 4;
    if (write(pipefd[1], &x, sizeof(x)) == -1) {
      perror("write");
      return 1;
    }
    printf("Sent x = %d\n", x);
    close(pipefd[1]);
    close(pipefd2[0]);
  } else {
    close(pipefd[1]);
    close(pipefd2[0]);
    srand(time(NULL));
    int x = rand() % 10;
    if (write(pipefd2[1], &x, sizeof(x)) == -1) {
      perror("write");
      return 1;
    }
    printf("Sent x = %d\n", x);
    if (read(pipefd[0], &x, sizeof(x)) == -1) {
      perror("read");
      return 1;
    }
    printf("Received x = %d\n", x);
    close(pipefd[0]);
    close(pipefd2[1]);
  }
  return 0;
}
