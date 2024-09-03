#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define TCP_PORT 5100 				/* 서버의 포트 번호 */

static int g_noc = 0;

void sigfunc(int no) 
{
    printf("Signal : %d(%d)\n", no, g_noc);
    if(--g_noc == 0) exit(0);
}

int main(int argc, char **argv)
{
    int ssock;  /* 소켓 디스크립트 정의 */
    socklen_t clen;
    pid_t pid;
    int n;
    struct sockaddr_in servaddr, cliaddr;  /* 주소 구조체 정의 */
    char mesg[BUFSIZ];

    portno = (argc == 2)?atoi(argv[1]):TCP_PORT;

    signal(SIGCHLD, sigfunc);

    /* 서버 소켓 생성 */
    if((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    /* 주소 구조체에 주소 지정 */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(TCP_PORT);  /* 사용할 포트 지정 */

    /* bind 함수를 사용하여 서버 소켓의 주소 설정 */
    unlink()
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind()");
        close(ssock);
        return -1;
    }

    /* 동시에 접속하는 클라이언트의 처리를 위한 대기 큐를 설정 */
    if(listen(ssock, 8) < 0) {
        perror("listen()");
        close(ssock);
        return -1;
    }

    clen = sizeof(cliaddr);
    while(1) {
        /* 클라이언트가 접속하면 접속을 허용하고 클라이언트 소켓 생성 */
        int csock = accept(ssock, (struct sockaddr *)&cliaddr, &clen);
        if(csock < 0) {
            perror("accept()");
            continue;
        }

        /* 네트워크 주소를 문자열로 변경 */
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client is connected : %s\n", mesg);

        if((pid = fork()) < 0) {
            perror("fork()");
            close(csock);
            continue;
        } else if(pid == 0) {  /* 자식 프로세스 */
            close(ssock);  /* 자식 프로세스에서 서버 소켓을 닫음 */

            while(1) {
                memset(mesg, 0, BUFSIZ);
                n = read(csock, mesg, BUFSIZ);
                if(n <= 0) {
                    if(n < 0) perror("read()");
                    break;  /* 클라이언트가 연결을 종료한 경우 루프 종료 */
                }

                printf("Received data : %s", mesg);

                /* 클라이언트로 buf에 있는 문자열 전송 */
                if(write(csock, mesg, n) <= 0) {
                    perror("write()");
                    break;
                }

                if(strncmp(mesg, "q", 1) == 0)
                    break;
            }

            close(csock);  /* 클라이언트 소켓을 닫음 */
            exit(0);  /* 자식 프로세스 종료 */
        } else {  /* 부모 프로세스 */
            close(csock);  /* 부모 프로세스에서는 클라이언트 소켓을 닫음 */
            // waitpid(pid, NULL, 0);  /* 자식 프로세스의 종료를 기다림 */
        }
    }

    close(ssock);  /* 서버 소켓을 닫음 */
    return 0;
}
