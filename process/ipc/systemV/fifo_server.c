#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>

#define FIFOFILE "fifo"

static void signalHandler(int);
static int fd;

int main(int argc, char** argv) {
    int n; 
    char buf[BUFSIZ];

    if (signal(SIGINT, signalHandler) < 0) {
        perror("signal error : SIGINT");
        return -1;
    }

    unlink(FIFOFILE); /* remove current FIFO */
    if(mkfifo(FIFOFILE, 0666) <0) { /* make new FIFO */
        perror("mkfifo()");
        return -1;
    }

    if((fd = open(FIFOFILE, O_RDONLY)) < 0 ) {
        perror("open()");
        return -1;
    }

    while((n = read(fd, buf, sizeof(buf))) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }

    if (n == 0) {
        printf("EOF reached\n");
    } else if (n < 0) {
        perror("read()");
    }
    
    close(fd);
    return 0;
}

static void signalHandler(int signo) {
    if (signo == SIGINT) {
        printf("catched SIGINT : %d\n", signo);
        close(fd);
    } 
}