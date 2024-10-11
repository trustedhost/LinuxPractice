/* raspberry pi 5 에서는 동작하지 않음. (64비트)*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#if 0
#define BCM_IO_BASE 0x20000000 /* Raspberry Pi B/B+? I/O Peripherals ?? */
#define BCM_IO_BASE 0x3F000000 /* Raspberry Pi 2/3? I/O Peripherals ?? */
#define BCM_IO_BASE 0xFE000000 /* Raspberry Pi 4 (32bit) I/O Peripherals ?? */
#else

#endif

#define GPIO_BASE (BCM_IO_BASE + 0x200000) /* GPIO ????? ?? */
#define GPIO_SIZE (256) /* 0x7E2000B0 ? 0x7E2000000 + 4 = 176 + 4 = 180 */

/* GPIO ?? ??? */
#define GPIO_IN(g)                                                             \
  (*(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))) /* ?? ?? */
#define GPIO_OUT(g)                                                            \
  (*(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))) /* ?? ?? */

#define GPIO_SET(g) (*(gpio + 7) = 1 << g)    /* ?? ?? */
#define GPIO_CLR(g) (*(gpio + 10) = 1 << g)   /* ??? ?? ?? */
#define GPIO_GET(g) (*(gpio + 13) & (1 << g)) /* ?? GPIO? ??? ?? ?? ?? */

volatile unsigned *gpio; /* I/O ??? ?? volatile ?? */

int main(int argc, char **argv) {
  int gno, i, mem_fd;
  void *gpio_map;

  if (argc < 2) {
    printf("Usage : %s GPIO_NO\n", argv[0]);
    return -1;
  }

  gno = atoi(argv[1]);
  /* /dev/mem ???? ?? */
  if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
    perror("open() /dev/mem\n");
    return -1;
  }

  /* GPIO? mmap */
  gpio_map = mmap(NULL, GPIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd,
                  GPIO_BASE);
  if (gpio_map == MAP_FAILED) {
    printf("[Error] mmap() : %d\n", (int)gpio_map);
    perror - 1;
  }

  gpio = (volatile unsigned *)gpio_map; /* ??? ?? ?? ??? */

  GPIO_OUT(gno); /* ?? GPIO ?? ???? ?? */

  for (i = 0; i < 5; i++) {
    printf("haha\n");
    GPIO_SET(gno); /* ?? GPIO ?? ? ?? */
    sleep(1);
    GPIO_CLR(gno); /* ?? GPIO ?? ? ?? */
    sleep(1);
  }

  munmap(gpio_map, GPIO_SIZE); /* ??? mmap ?? ?? */
  close(mem_fd);

  return 0;
}