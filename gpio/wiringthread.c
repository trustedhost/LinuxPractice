#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <softPwm.h>
#include <softTone.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <wiringPi.h>

static pthread_mutex_t music_lock;

static int is_run = 1;
static _Bool is_play = false;

#define SPKR 6  /* GPIO25 */
#define TOTAL 1 /* Total number of notes */

#define SW 5  /* GPIO pin for switch */
#define LED 1 /* GPIO pin for LED */

int kbhit(void);

int notes[] = {
    391 /* Add the rest of the notes here */
};

void *musicPlay() {
  while (is_run) {
    softToneCreate(SPKR);         /* Setup tone output */
    softPwmCreate(SPKR, 0, 2000); /* PWM range setup */

    /* Play music if is_play is true */
    if (pthread_mutex_trylock(&music_lock) == 0) {
      if (is_play) {
        for (int i = 0; i < TOTAL; ++i) {
          softToneWrite(SPKR, notes[i]); /* Play note */
          softPwmWrite(SPKR, 100);       /* Adjust PWM for volume */
          delay(280);                    /* Note duration */
        }
      }
      pthread_mutex_unlock(&music_lock);
    }

    /* Stop the sound */
    softToneWrite(SPKR, 0);
    softPwmWrite(SPKR, 0);
    delay(10); /* Short delay to avoid high CPU usage */
  }
  return NULL;
}

void *switchControl() {
  pinMode(SW, INPUT);
  pinMode(LED, OUTPUT);
  pullUpDnControl(SW, PUD_UP); /* Enable pull-up resistor */

  while (is_run) {
    if (digitalRead(SW) == LOW) {   /* Check if switch is pressed */
      delay(20);                    /* Debounce delay */
      if (digitalRead(SW) == LOW) { /* Confirm switch press */
        digitalWrite(LED, HIGH);    /* LED on */
        if (pthread_mutex_lock(&music_lock) == 0) {
          is_play = !is_play; /* Toggle play state */
          printf("is_play changed to: %d\n", is_play);
          pthread_mutex_unlock(&music_lock);
        }
        while (digitalRead(SW) ==
               LOW) { /* Wait for switch release */ /*genius strategy*/
          delay(10);
        }
        digitalWrite(LED, LOW); /* LED off */
      }
    }
    delay(50); /* Short delay to avoid high CPU usage */
  }
  return NULL;
}

int main() {
  wiringPiSetup(); /* Setup wiringPi */

  pthread_t ptSwitch, ptMusic;
  pthread_mutex_init(&music_lock, NULL);

  pthread_create(&ptSwitch, NULL, switchControl, NULL);
  pthread_create(&ptMusic, NULL, musicPlay, NULL);

  printf("Press 'q' to quit\n");
  while (is_run) {
    if (kbhit()) {
      char ch = getchar();
      if (ch == 'q') {
        is_run = 0;
        break;
      }
    }
    delay(100); /* Avoid high CPU usage in the main loop */
  }

  pthread_join(ptSwitch, NULL);
  pthread_join(ptMusic, NULL);

  pthread_mutex_destroy(&music_lock);
  printf("Program ended\n");
  return 0;
}

/* Check if a keyboard key is hit */
int kbhit(void) {
  struct termios oldt, newt;
  int ch, oldf;

  tcgetattr(STDIN_FILENO, &oldt); /* Get current terminal settings */
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);        /* Disable canonical mode and echo */
  tcsetattr(STDIN_FILENO, TCSANOW, &newt); /* Apply new settings */
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK); /* Set non-blocking mode */

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt); /* Restore terminal settings */
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin); /* Push character back to input buffer */
    return 1;
  }
  return 0;
}
