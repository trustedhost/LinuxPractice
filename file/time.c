#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

int main() {
    int i, j;
    time_t rawtime;
    struct tm *tm;
    char buf[BUFSIZ];
    struct timeval mytime;

    time(&rawtime);
    printf("time : %u\n", (unsigned)rawtime);

    gettimeofday(&mytime, NULL);
    printf("gettimeofday : %ld/%ld\n", mytime.tv_sec, mytime.tv_usec);

    printf("cime : %s\n", ctime(&rawtime));

    putenv("TZ=PST3PDT");
    tzset();
    tm = localtime(&rawtime);
    printf("asctime : %s\n", asctime(tm));

    strftime(buf, sizeof(buf), "%A %d %B %I:%M:%S %p", tm);
    printf("strftime : %s\n", buf);

    return 0;
}