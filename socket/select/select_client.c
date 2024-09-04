#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <arpa/inet.h> 
#include <string.h>

#define TCP_PORT 5100

int main(int argc, char** argv) {
    int sockfd;
    fd_set readfds;
    char mesg[BUFSIZ];

    if(argc < 2) {
        printf("Usage : %s IP_ADDRESS\n", argv[0]);
        return -1;
    }

    /* 소켓 생성 및 연결 */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket()");
        return 1;  
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, argv[1], &(server_addr.sin_addr.s_addr));
    server_addr.sin_port = htons(TCP_PORT);


    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect()");
        return 1;
    }
    
    while (1) {
        FD_ZERO(&readfds); /* readfds 를 0으로 초기화 ( 지역변수로 선언했기 때문에 데이터 초기화가 필요하다. )*/
        FD_SET(0, &readfds);
        FD_SET(sockfd, &readfds);

        /* 지켜볼 file descriptor = sockfd*/
        int maxfd = sockfd > 0 ? sockfd : 0;
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL); /* 시간을 지정해주지 않았기 때문에, (마지막 인자 없음) -> 무한히 기다린다. */
        if (activity < 0) {
            perror("select()");
            break;
        }
        /* stdin에서 변화가 생기면 sockfd로 전송 (클라이언트 -> 서버) */
        if(FD_ISSET(0, &readfds)) { 
            int size = read(0, mesg, BUFSIZ);
            if (size > 0) {
                write(sockfd, mesg, size);
            } else {
                break; /* EOF (Ctrl+D) 입력 시 종료 */
            }
        }

        /* sockfd에서 변화가 생기면 stdout으로 전송 (서버 -> 클라이언트) */
        if(FD_ISSET(sockfd, &readfds)) { 
            int size = read(sockfd, mesg, BUFSIZ);
            if (size > 0) {
                write(1, mesg, size);
            } else {
                break; /* 서버가 연결을 끊었을 때 종료 */
            }
        }
    }
    close(sockfd);
    return 0;
}