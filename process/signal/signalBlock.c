#include <stdio.h>
#include <signal.h>
#include <unistd.h>

// SIGINT 시그널 핸들러 함수 (Ctrl+C)
void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received SIGINT (Ctrl+C)\n");
    } else if (signo = SIGUSR1) {
        printf("Received SIGUSR1\n");
    };
}

int main() {
    sigset_t newmask, oldmask;

    // SIGINT 시그널 핸들러 등록
    signal(SIGINT, signal_handler);
    signal(SIGUSR1, signal_handler);

    // SIGINT를 블록하는 시그널 집합 설정
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGINT);
    sigaddset(&newmask, SIGUSR1);

    // SIGINT 블록 적용, 기존 마스크는 oldmask에 저장
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    printf("SIGINT is blocked. Press Ctrl+C, but it won't stop the program.\n");
    printf("Sleeping for 10 seconds...\n");

    // SIGINT 블록된 상태에서 10초 동안 대기
    sleep(15);
    // 블록을 해제하고 이전 마스크(oldmask)를 복원
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
    
    // 블록된 시그널 
    // sigpending(&pending);
    // if (sigismember(&pending, SIGINT)) {
    //     printf("SIGINT is pending.\n");
    // } else {
    //     printf("SIGINT is not pending.\n");
    // }

    printf("SIGINT is unblocked. If you pressed Ctrl+C before, it will be handled now.\n");

    // 10초 동안 기다리며 시그널 처리를 대기
    sleep(3);

    printf("Program finished.\n");

    return 0;
}
