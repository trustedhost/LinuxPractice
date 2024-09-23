#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int arr[] = {1, 2, 3, 4, 5, 6, 7};
  int arrSize = sizeof(arr) / sizeof(arr[0]);
  int start, end;
  int fd[2];
  if (pipe(fd) == -1) {
    perror("pipe");
    return 1;
  }
  int id = fork();
  if (id == -1) {
    perror("fork");
    return 1;
  }
  if (id == 0) {
    start = 0;
    end = arrSize;
  } else {
    start = arrSize / 2;
    end = arrSize;
  }

  int sum = 0;
  int i;
  for (i = start; i < end; i++) {
    sum += arr[i];
  }

  printf("Calculated partial sum : %d\n", sum);

  if (id == 0) {
    close(fd[0]);
    write(fd[1], &sum, sizeof(sum));
    close(fd[1]);
  } else {
    close(fd[1]);
    int childSum;
    if (read(fd[0], &childSum, sizeof(childSum)) == -1) {
      perror("read");
      return 1;
    }
    sum += childSum;
    close(fd[0]);
    printf("Total sum : %d\n", sum);
    while (wait(NULL) != -1 || errno != ECHILD) {
      printf("Waiting for child process to finish\n");
    }
  }

  return 0;
}