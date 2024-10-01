#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <netinet/in.h>
#include <pthread.h>
#include <softPwm.h>
#include <softTone.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wiringPi.h>

#define SPKR 6  // GPIO25
#define SW 5    // GPIO pin for switch
#define LED 1   // GPIO pin for LED
#define CDS 0   // GPIO pin for CDS (light sensor)
#define MOTOR 2 // GPIO pin for motor

#define TOTAL 8 // Total number of notes

static pthread_mutex_t state_lock;
static int is_run = 1;

int notes[] = {391, 440, 493, 523, 587, 659, 698, 784};

typedef struct {
  bool is_alarm_on;
  bool is_room_bright;
  bool is_led_on;
  bool is_motor_on;
  int volume;
  time_t last_update;
} SystemState;

SystemState system_state = {false, false, false, false, 50, 0};

int kbhit(void);
void *musicPlay(void *arg);
void *switchControl(void *arg);
void *lightSensor(void *arg);
void *webserverFunction(void *arg);
void *clnt_connection(void *arg);
void sendOk(FILE *fp);
void sendError(FILE *fp);
void sendData(FILE *fp);

void *musicPlay(void *arg) {
  softToneCreate(SPKR);
  while (is_run) {
    pthread_mutex_lock(&state_lock);
    if (system_state.is_alarm_on && system_state.is_room_bright) {
      int volume = system_state.volume;
      pthread_mutex_unlock(&state_lock);
      for (int i = 0; i < TOTAL && is_run; ++i) {
        pthread_mutex_lock(&state_lock);
        if (!system_state.is_alarm_on || !system_state.is_room_bright) {
          pthread_mutex_unlock(&state_lock);
          break;
        }
        int adjusted_frequency = notes[i] * volume / 100;
        softToneWrite(SPKR, adjusted_frequency);
        pthread_mutex_unlock(&state_lock);
        delay(280);
      }
    } else {
      softToneWrite(SPKR, 0);
      pthread_mutex_unlock(&state_lock);
    }
    delay(10);
  }
  return NULL;
}

void *switchControl(void *arg) {
  pinMode(SW, INPUT);
  pinMode(LED, OUTPUT);
  pullUpDnControl(SW, PUD_UP);

  while (is_run) {
    if (digitalRead(SW) == LOW) {
      delay(20);
      if (digitalRead(SW) == LOW) {
        pthread_mutex_lock(&state_lock);
        system_state.is_alarm_on = !system_state.is_alarm_on;
        system_state.last_update = time(NULL);
        if (!system_state.is_alarm_on) {
          softToneWrite(SPKR, 0);
        }
        pthread_mutex_unlock(&state_lock);
        while (digitalRead(SW) == LOW) {
          delay(10);
        }
      }
    }
    delay(50);
  }
  return NULL;
}

void *lightSensor(void *arg) {
  pinMode(CDS, INPUT);
  pinMode(LED, OUTPUT);

  while (is_run) {
    int light_level = digitalRead(CDS);
    pthread_mutex_lock(&state_lock);
    system_state.is_room_bright = (light_level == HIGH);

    if (!system_state.is_room_bright || system_state.is_alarm_on) {
      digitalWrite(LED, HIGH);
      system_state.is_led_on = true;
    } else {
      digitalWrite(LED, LOW);
      system_state.is_led_on = false;
    }

    system_state.last_update = time(NULL);
    pthread_mutex_unlock(&state_lock);
    delay(100);
  }
  return NULL;
}

void *webserverFunction(void *arg) {
  int ssock, port = *((int *)arg);
  struct sockaddr_in servaddr, cliaddr;
  socklen_t len;

  ssock = socket(AF_INET, SOCK_STREAM, 0);
  if (ssock == -1) {
    perror("socket error");
    return NULL;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  int opt = 1;
  if (setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    return NULL;
  }

  if (bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
    perror("bind error");
    return NULL;
  }

  if (listen(ssock, 10) == -1) {
    perror("listen error");
    return NULL;
  }

  printf("Server listening on port %d\n", port);

  while (is_run) {
    int csock;
    len = sizeof(cliaddr);
    csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
    if (csock == -1) {
      perror("accept error");
      continue;
    }

    char mesg[BUFSIZ];
    inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
    printf("Received connection from %s:%d\n", mesg, ntohs(cliaddr.sin_port));

    pthread_t thread;
    pthread_create(&thread, NULL, clnt_connection, &csock);
  }
  close(ssock);
  return NULL;
}

void *clnt_connection(void *arg) {
  int csock = *((int *)arg);
  FILE *clnt_read, *clnt_write;
  char request[BUFSIZ], method[10], path[255], protocol[10];

  clnt_read = fdopen(csock, "r");
  clnt_write = fdopen(dup(csock), "w");

  if (fgets(request, BUFSIZ, clnt_read) == NULL) {
    fclose(clnt_read);
    fclose(clnt_write);
    return NULL;
  }

  sscanf(request, "%s %s %s", method, path, protocol);
  printf("Request: %s %s %s\n", method, path, protocol);

  if (strcmp(method, "GET") != 0) {
    sendError(clnt_write);
    fclose(clnt_read);
    fclose(clnt_write);
    return NULL;
  }

  char *query = strchr(path, '?');
  if (query != NULL) {
    *query = '\0';
    query++;

    char *token = strtok(query, "&");
    while (token != NULL) {
      char *eq = strchr(token, '=');
      if (eq != NULL) {
        *eq = '\0';
        char *name = token;
        char *value = eq + 1;

        pthread_mutex_lock(&state_lock);
        if (strcmp(name, "alarm") == 0) {
          system_state.is_alarm_on = (strcmp(value, "on") == 0);
          if (!system_state.is_alarm_on) {
            softToneWrite(SPKR, 0);
          }
          system_state.last_update = time(NULL);
        } else if (strcmp(name, "motor") == 0) {
          system_state.is_motor_on = (strcmp(value, "on") == 0);
          system_state.last_update = time(NULL);
          // TODO: Implement motor control
          if (system_state.is_motor_on) {
            digitalWrite(MOTOR, HIGH);
          } else {
            digitalWrite(MOTOR, LOW);
          }
        } else if (strcmp(name, "volume") == 0) {
          int new_volume = atoi(value);
          if (new_volume >= 0 && new_volume <= 100) {
            system_state.volume = new_volume;
            system_state.last_update = time(NULL);
          }
        }
        pthread_mutex_unlock(&state_lock);
      }
      token = strtok(NULL, "&");
    }
  }

  sendData(clnt_write);

  fclose(clnt_read);
  fclose(clnt_write);
  return NULL;
}

void sendData(FILE *fp) {
  char protocol[] = "HTTP/1.0 200 OK\r\n";
  char server[] = "Server: Linux Web Server \r\n";
  char cnt_type[] = "Content-type: text/html\r\n\r\n";

  fputs(protocol, fp);
  fputs(server, fp);
  fputs(cnt_type, fp);

  pthread_mutex_lock(&state_lock);
  char time_str[64];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S",
           localtime(&system_state.last_update));

  fprintf(fp,
          "<html><head><title>Raspberry Pi Controller</title>"
          "<script>"
          "function refreshStatus() {"
          "  fetch('/?refresh=true')"
          "    .then(response => response.text())"
          "    .then(html => {"
          "      document.body.innerHTML = html;"
          "    });"
          "}"
          "function updateVolume() {"
          "  var volume = document.getElementById('volume').value;"
          "  fetch('/?volume=' + volume)"
          "    .then(response => response.text())"
          "    .then(html => {"
          "      document.body.innerHTML = html;"
          "    });"
          "}"
          "</script>"
          "</head>"
          "<body>"
          "<h1>Raspberry Pi Alarm System</h1>"
          "<p>Room brightness: %s</p>"
          "<p>LED status: %s</p>"
          "<p>Volume: %d%%</p>"
          "<p>Last update: %s</p>"
          "<form action=\"/\" method=\"GET\">"
          "<label><input type=\"radio\" name=\"alarm\" value=\"on\" %s> Alarm "
          "On</label>"
          "<label><input type=\"radio\" name=\"alarm\" value=\"off\" %s> Alarm "
          "Off</label><br>"
          "<label><input type=\"radio\" name=\"motor\" value=\"on\" %s> Motor "
          "On</label>"
          "<label><input type=\"radio\" name=\"motor\" value=\"off\" %s> Motor "
          "Off</label><br>"
          "<label for=\"volume\">Volume:</label>"
          "<input type=\"range\" id=\"volume\" name=\"volume\" min=\"0\" "
          "max=\"100\" value=\"%d\" onchange=\"updateVolume()\">"
          "<input type=\"submit\" value=\"Update\">"
          "</form>"
          "<button onclick=\"refreshStatus()\">Refresh Status</button>"
          "</body></html>",
          system_state.is_room_bright ? "Bright" : "Dark",
          system_state.is_led_on ? "On" : "Off", system_state.volume, time_str,
          system_state.is_alarm_on ? "checked" : "",
          !system_state.is_alarm_on ? "checked" : "",
          system_state.is_motor_on ? "checked" : "",
          !system_state.is_motor_on ? "checked" : "", system_state.volume);
  pthread_mutex_unlock(&state_lock);

  fflush(fp);
}

void sendOk(FILE *fp) {
  char protocol[] = "HTTP/1.1 200 OK\r\n";
  char server[] = "Server: Linux Web Server \r\n\r\n";

  fputs(protocol, fp);
  fputs(server, fp);
  fflush(fp);
}

void sendError(FILE *fp) {
  char protocol[] = "HTTP/1.1 400 Bad Request\r\n";
  char server[] = "Server: Linux Web Server\r\n";
  char cnt_len[] = "Content-Length: 111\r\n";
  char cnt_type[] = "Content-Type: text/html\r\n\r\n";
  char content[] =
      "<html><head><title>Bad Request</title></head><body><h1>400 Bad "
      "Request</h1><p>Your browser sent an invalid request.</p></body></html>";

  fputs(protocol, fp);
  fputs(server, fp);
  fputs(cnt_len, fp);
  fputs(cnt_type, fp);
  fputs(content, fp);
  fflush(fp);
}

int kbhit(void) {
  struct termios oldt, newt;
  int ch, oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  wiringPiSetup();

  pthread_t ptSwitch, ptMusic, ptLight, ptWeb;
  pthread_mutex_init(&state_lock, NULL);

  int port = atoi(argv[1]);

  pthread_create(&ptSwitch, NULL, switchControl, NULL);
  pthread_create(&ptMusic, NULL, musicPlay, NULL);
  pthread_create(&ptLight, NULL, lightSensor, NULL);
  pthread_create(&ptWeb, NULL, webserverFunction, &port);

  printf("Server started. Connect to http://127.0.0.1:%d\n", port);
  printf("Press 'q' to quit\n");

  while (is_run) {
    if (kbhit()) {
      char ch = getchar();
      if (ch == 'q') {
        is_run = 0;
        break;
      }
    }
    delay(100);
  }
}