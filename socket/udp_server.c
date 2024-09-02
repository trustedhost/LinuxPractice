#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h> // for inet_ntoa


#define UDP_PORT 5100

int main(int argc, char **argv) {
    int sockfd, n;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t len;
    char mesg[100];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); /*udp socket genrate*/

    /* set address and register service to OS.*/
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(UDP_PORT);
    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    do {
        len = sizeof(cliaddr);
        n = recvfrom(sockfd, mesg, sizeof(mesg) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        sendto(sockfd, mesg, n, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
        mesg[n] = '\0';
        printf("Received data from(%s) : %s\n",inet_ntoa(cliaddr.sin_addr),mesg);
    } while (strncmp(mesg, "q", 1));

    close(sockfd);

    return 0;
}