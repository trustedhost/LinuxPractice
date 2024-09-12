#include <stdio.h>
#include <unistd.h>

int main() {
    int a;
    close(0); // fd 0 을 닫게되면, scanf가 동작하지 않는다. 
    scanf("%d", &a);            //fd: 0 (stdin)
    printf("Hello World\n");    //fd: 1 (stdout)
    perror("Hello Error");      //fd: 2 (stderr)
    return 0;
}