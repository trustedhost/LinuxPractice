#include <stdio.h>
#include <stdlib.h>
#include <time.h>
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
    srand(time(NULL));
    int arr[5], i, n;
    n = sizeof(arr) / sizeof(arr[0]);
    for (i = 0; i < 5; i++) {
      arr[i] = rand() % 10;
    };
    close(fd[0]);
    write(fd[1], &n, sizeof(n));
    write(fd[1], arr, sizeof(arr));
    close(fd[1]);
  } else {
    int arr[5], i, n;
    close(fd[1]);
    if (read(fd[0], &n, sizeof(n)) == -1) {
      perror("read");
      return 1;
    }
    if (read(fd[0], arr, sizeof(arr)) == -1) {
      perror("read");
      return 1;
    }
    close(fd[0]);
    for (i = 0; i < n; i++) {
      printf("arr[%d] = %d\n", i, arr[i]);
    }
  }

  return 0;
}