#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#define FIFOFILE "fifo"
static void printSigset(sigset_t *set);
static void sigHandler(int);
int fd;

int main(int argc, char **argv) {
    int n;
    char buf[BUFSIZ];

    sigset_t pset;
    sigemptyset(&pset);
    sigaddset(&pset, SIGQUIT);
    sigaddset(&pset, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &pset, NULL);

    printSigset(&pset);

    if(signal(SIGINT, sigHandler) == SIG_ERR) {
        perror("signal() : SIGINT");
        return -1;
    }
    if(signal(SIGPIPE, sigHandler) == SIG_ERR) {
        perror("signal() : SIGPIPE");
        return -1;
    }
    if((fd = open(FIFOFILE, O_WRONLY)) < 0) {
        perror("open()");
        return -1;
    }
    // 1초마다 "Hello World\n" 메시지를 FIFO로 전송
    while (1) {
        sleep(1);  // 1초 대기
        if (write(fd, "Hello World\n", 13) == -1) {
            perror("write()");
            break;
        }
    }
    // while ((n = read(0, buf, sizeof(buf))) > 0) {
    //     write(fd, buf, n);
    // }
    // while (1) {
    //     sleep(1000);
    //     write(fd, "Hello World\n", 13);
    // }

    close(fd);
    return 0;
}


static void sigHandler(int signo) {
    if(signo == SIGINT) {
        printf("SIGINT is catched : %d\n", signo);
        exit(0);
    } else if (signo == SIGPIPE) {
        printf("SIGPIPE is catched : %d\n", signo);
    }
}


static void printSigset(sigset_t *set) {
    int i;
    for(i = 1; i < NSIG; ++i) {
        printf((sigismember(set, i)) ? "1" : "0");
    }
    putchar('\n');
}