#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char** argv) {
    char line[BUFSIZ]; /* BUFSIZ defined in stdio.h */
    FILE *pf = popen("/bin/date", "r");
    if (!pf) {
        perror("popen()");
        return -1;
    }
    while (fgets(line, BUFSIZ, pf)) {
        printf("Read from parent process [pid : %d]", getpid());
        printf("%s", line);
    }
    pclose(pf);
}