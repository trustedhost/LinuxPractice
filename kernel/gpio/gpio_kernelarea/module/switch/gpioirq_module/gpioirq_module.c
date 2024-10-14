#include <linux/cdev.h> /* 문자 디바이스 */
#include <linux/fs.h>   /* open( ), read( ), write( ), close( ) 커널 함수 */
#include <linux/gpio.h> /* GPIO 함수 */
#include <linux/interrupt.h> /* 인터럽트 처리를 위한 헤더 파일 */
#include <linux/io.h>        /* ioremap( ), iounmap( ) 커널 함수 */
#include <linux/module.h>
#include <linux/uaccess.h> /* copy_to_user( ), copy_from_user( ) 커널 함수 */

MODULE_LICENSE("GPL");
/* 디바이스 파일의 주 번호와 부 번호 */
#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpioled" /* 디바이스 디바이스 파일의 이름 */

#define GPIO_LED 589 /* LED 사용을 위한 GPIO의 번호 */
#define GPIO_SW 595  /* 스위치에 대한 GPIO의 번호 */

static char msg[BLOCK_SIZE] = {0}; /* write( ) 함수에서 읽은 데이터 저장 */
struct cdev gpio_cdev;
static int switch_irq;

/* 인터럽트 처리를 위한 인터럽트 서비스 루틴(Interrupt Service Routine) */
static irqreturn_t isr_func(int irq, void *data) {
  if (irq == switch_irq && !gpio_get_value(GPIO_LED)) {
    gpio_set_value(GPIO_LED, 1);
  } else if (irq == switch_irq && gpio_get_value(GPIO_LED)) {
    gpio_set_value(GPIO_LED, 0);
  }

  return IRQ_HANDLED;
}

int init_module(void) {
  dev_t devno;

  printk("'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
  printk("'chmod 666 /dev/%s'\n", GPIO_DEVICE);

  /* GPIO 사용을 요청한다. */
  gpio_request(GPIO_LED, "LED");
  gpio_direction_output(GPIO_LED, 0);
  gpio_request(GPIO_SW, "SWITCH");
  gpio_direction_input(GPIO_SW);
  switch_irq = gpio_to_irq(GPIO_SW); /* GPIO 인터럽트 번호 획득 */
  int err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch",
                        NULL); /* GPIO 인터럽트 핸들러 등록 */

  return 0;
}

void cleanup_module(void) {
  dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
  unregister_chrdev_region(devno, 1); /* 문자 디바이스의 등록을 해제한다. */

  cdev_del(&gpio_cdev); /* 문자 디바이스의 구조체를 해제한다. */

  /* 사용이 끝난 인터럽트 해제 */
  free_irq(switch_irq, NULL);

  /* 더 이상 사용이 필요 없는 경우 관련 자원을 해제한다. */
  gpio_free(GPIO_LED);
  gpio_free(GPIO_SW);
  // gpio_direction_output(GPIO_LED, 0);

  module_put(THIS_MODULE);

  printk(KERN_INFO "Good-bye module!\n");
}
