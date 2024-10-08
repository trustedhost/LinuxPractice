#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* threading */
static void *clnt_connection(void *arg);
int sendData(FILE* fp, char *ct, char *filename);
void sendOk(FILE* fp);
void sendError(FILE* fp, const char *message);

int main(int argc, char **argv)
{
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    if (argc != 2) {
        printf("usage : %s <port>\n", argv[0]);
        return -1;
    }

    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
        perror("socket()");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = (argc != 2) ? htons(8000) : htons(atoi(argv[1]));
    if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        return -1;
    }

    if (listen(ssock, 10) == -1 ){
        perror("listen()");
        return -1;
    }

    while(1) {
        char mesg[BUFSIZ];
        int *csock = malloc(sizeof(int));  // 동적 할당된 소켓 포인터
        len = sizeof(cliaddr);
        *csock = accept(ssock, (struct sockaddr*)&cliaddr, &len);

        if (*csock == -1) {
            perror("accept()");
            free(csock);  // 오류 발생 시 메모리 해제
            continue;
        }

        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("Client IP : %s:%d\n", mesg, ntohs(cliaddr.sin_port));

        pthread_create(&thread, NULL, clnt_connection, csock);
        pthread_detach(thread);  // 스레드를 분리하여 리소스 누수를 방지
    }
    return 0;
}

void *clnt_connection(void *arg) {
    int csock = *((int*)arg);
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], method[BUFSIZ], filename[BUFSIZ], *ret;
    char host[BUFSIZ] = {0};  // Host 헤더 값을 저장할 변수

    free(arg);  // 동적 할당된 소켓 포인터 해제

    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    if (fgets(reg_line, BUFSIZ, clnt_read) == NULL) {
        sendError(clnt_write, "Failed to read request");
        goto END;
    }

    fputs(reg_line, stdout);

    ret = strtok(reg_line, " ");
    strcpy(method, (ret != NULL) ? ret : "");

    if (strcmp(method, "POST") == 0) {
        sendOk(clnt_write);
        goto END;
    } else if (strcmp(method, "GET") != 0) {
        sendError(clnt_write, "Invalid method");
        goto END;
    }

    ret = strtok(NULL, " ");
    if (ret == NULL || ret[0] != '/') {
        sendError(clnt_write, "Invalid filename");
        goto END;
    }

    strcpy(filename, ret + 1);  // '/' 이후의 파일명만 복사


/* solution 1
    // 헤더 부분을 계속 읽어서 Host 헤더를 찾음
    while (fgets(reg_line, BUFSIZ, clnt_read) != NULL && strcmp(reg_line, "\r\n") != 0) {
        if (strncmp(reg_line, "Host:", 5) == 0) {
            // Host 헤더 값을 추출
            strcpy(host, reg_line + 6);
            // 개행 문자 제거
            host[strcspn(host, "\r\n")] = 0;
            printf("Host: %s\n", host);  // Host 값을 출력
        }
    }
 */

// solution 2 - strtok 사용
    while (fgets(reg_line, BUFSIZ, clnt_read) != NULL && strcmp(reg_line, "\r\n") != 0) {
        char *token = strtok(reg_line, " :\r\n");
        // "Host" 헤더가 있는 지 확인
        if (token != NULL&& strcmp(token, "Host") == 0){
            token = strtok(NULL, " \r\n");

            if (token != NULL) {
                strcpy(host, token);
                printf("Host : %s\n", host);
            }
        }
    }

    // "User-Agent" 헤더가 있는지 확인

    if (access(filename, F_OK) == -1) {  // 파일 존재 여부 확인
        sendError(clnt_write, "File not found");
        goto END;
    }

    sendData(clnt_write, "text/html", filename);

END:
    fclose(clnt_read);
    fclose(clnt_write);
    pthread_exit(0);
    return NULL;
}

int sendData(FILE* fp, char *ct, char *filename) {
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server: SimpleServer\r\n";
    char cnt_type[] = "Content-Type:text/html\r\n";
    char end[] = "\r\n";
    char buf[BUFSIZ];
    int fd, len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open()");
        return -1;
    }

    while ((len = read(fd, buf, BUFSIZ)) > 0) {
        fwrite(buf, sizeof(char), len, fp);
    }

    close(fd);
    fflush(fp);
    return 0;
}

void sendOk(FILE* fp) {
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server: SimpleServer\r\n\r\n";
    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}

void sendError(FILE* fp, const char *message) {
    char protocol[] = "HTTP/1.1 400 Bad Request\r\n";
    char server[] = "Server: SimpleServer\r\n";
    char cnt_type[] = "Content-Type:text/html\r\n\r\n";
    char content[BUFSIZ];

    snprintf(content, BUFSIZ, "<html><head><title>Error</title></head><body><h1>%s</h1></body></html>", message);

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(content, fp);
    fflush(fp);
}
