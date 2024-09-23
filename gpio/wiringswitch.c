#include <wiringPi.h>
#include <stdio.h>

#define SW 5
#define LED 1

int switchControl() 
{
	int i;
	pinMode(SW, INPUT);
	pinMode(LED, OUTPUT);

	for(;;) {
    if (digitalRead(SW) == LOW) {
        printf("Switch pressed\n");
        digitalWrite(LED, HIGH);
        delay(1000);
        digitalWrite(LED, LOW);
    } else {
        printf("Switch not pressed\n");
    }
    delay(10);
	}
	return 0;
}

int main(int argc, char **argv) {
    if (wiringPiSetup() == -1) {
    printf("Setup failed!\n");
    return 1;
	}

	switchControl();
	return 0;
}
