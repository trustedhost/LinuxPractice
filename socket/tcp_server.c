#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_ntop

#define TCP_PORT 5100

int main(int argc, char **argv) {
    int ssock; /* socket descriptor */
    socklen_t clen;
    struct sockaddr_in servaddr, cliaddr;
    char mesg[BUFSIZ];

    if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    /* set address and register service to OS. */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);

    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        close(ssock);
        return -1;
    }

    if(listen(ssock, 8) < 0) {
        perror("listen()");
        close(ssock);
        return -1;
    }
    
    clen = sizeof(cliaddr);

    while (1) {
        int csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if (csock < 0) {
            perror("accept()");
            close(ssock);
            return -1;
        }

        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is connected : %s\n", mesg);

        while (1) {
            int n = read(csock, mesg, BUFSIZ);
            if (n <= 0) {
                // 클라이언트가 연결을 종료하거나 오류가 발생한 경우
                perror("read()");
                close(csock);
                break;
            }

            mesg[n] = '\0';  // null-terminate the received string
            printf("Received data : %s", mesg);

            if(write(csock, mesg, n) <= 0) {
                perror("write()");
                close(csock);
                break;
            }

            if (strncmp(mesg, "q", 1) == 0) {
                printf("Client requested shutdown.\n");
                close(csock);
                break;
            }
        }
    }

    close(ssock);
    return 0;
}
