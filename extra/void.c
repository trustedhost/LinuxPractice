/* void 가 argument 자리에 오면, 인자가 없다는 의미를 가진다. */
#include <stdio.h>

int add1() {
    return 0;
}
int add2(void) {
    return 0;
}

int main() {
    printf("%d\n", add1(1)); // no error
    printf("%d\n", add2(1)); // error

}