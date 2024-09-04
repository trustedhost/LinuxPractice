#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h>

int main(int argc, char** argv) {
    fd_set readfds;
    char mesg[BUFSIZ];

    FD_ZERO(&readfds);
    FD_SET(0, &readfds);

    /* 지켜볼 file descriptor = sockfd*/
    select(0 + 1, &readfds, NULL, NULL, NULL); 

    /* stdin 에서  변화가 생기면 stdout 로 전송해라. */
    if(FD_ISSET(0, &readfds)) { 
        int size = read(0, mesg, BUFSIZ);
        write(1, mesg, size);
    }
}