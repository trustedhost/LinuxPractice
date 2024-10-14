#include <linux/cdev.h> /* 문자 디바이스 */
#include <linux/fs.h>   /* open( ), read( ), write( ), close( ) 커널 함수 */
#include <linux/gpio.h> /* GPIO 함수 */
#include <linux/interrupt.h> /* 인터럽트 처리를 위한 헤더 파일 */
#include <linux/io.h>        /* ioremap( ), iounmap( ) 커널 함수 */
#include <linux/module.h>
#include <linux/uaccess.h> /* copy_to_user( ), copy_from_user( ) 커널 함수 */

#include <linux/mutex.h>
#include <linux/timer.h>

#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YoungJin Suh");
MODULE_DESCRIPTION("Raspberry Pi GPIO LED Device Module");

/* 디바이스 파일의 주 번호와 부 번호 */
#define GPIO_MAJOR 200
#define GPIO_MINOR 0
#define GPIO_DEVICE "gpioled" /* 디바이스 디바이스 파일의 이름 */

/* 디바이스 번호 : cat /sys/kernel/debug/gpio */
#define GPIO_LED                                                               \
  589               // 18 			/* LED 사용을 위한 GPIO의 번호
                    // */
#define GPIO_SW 595 // 24 			/* 스위치에 대한 GPIO의 번호 */

static char msg[BLOCK_SIZE] = {0}; /* write( ) 함수에서 읽은 데이터 저장 */

/* 입출력 함수를 위한 선언 */
static int gpio_open(struct inode *, struct file *);
static ssize_t gpio_read(struct file *, char *, size_t, loff_t *);
static ssize_t gpio_write(struct file *, const char *, size_t, loff_t *);
static int gpio_close(struct inode *, struct file *);

/* 유닉스 입출력 함수들의 처리를 위한 구조체 */
static struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .read = gpio_read,
    .write = gpio_write,
    .open = gpio_open,
    .release = gpio_close,
};

struct cdev gpio_cdev;
static int switch_irq;
static struct timer_list timer;
static struct task_struct *task;
static DEFINE_MUTEX(led_mutex);

static void timer_func(struct timer_list *t) {
  if (mutex_trylock(&led_mutex) != 0) {
    static int ledflag = 1;
    gpio_set_value(GPIO_LED, ledflag);
    ledflag = !ledflag;
    mutex_unlock(&led_mutex);
  }
  mod_timer(&timer, jiffies + (1 * HZ));
}

/* 인터럽트 처리를 위한 인터럽트 서비스 루틴(Interrupt Service Routine) */
static irqreturn_t isr_func(int irq, void *data) {
  if (mutex_trylock(&led_mutex) != 0) { /* 뮤텍스를 이용한 충돌 처리 */
    if (irq == switch_irq && !gpio_get_value(GPIO_LED)) {
      gpio_set_value(GPIO_LED, 1);
    } else if (irq == switch_irq && gpio_get_value(GPIO_LED)) {
      static struct kernel_siginfo sinfo;
      memset(&sinfo, 0, sizeof(struct kernel_siginfo));
      sinfo.si_signo = SIGIO;
      sinfo.si_code = SI_USER;
      send_sig_info(SIGIO, &sinfo, task);
      gpio_set_value(GPIO_LED, 0);
    }
    mutex_unlock(&led_mutex);
  }

  return IRQ_HANDLED;
}

int init_module(void) {
  dev_t devno;
  unsigned int count;
  int err;

  printk(KERN_INFO "Hello module!\n");

  mutex_init(&led_mutex);
  try_module_get(THIS_MODULE);

  /* 문자 디바이스를 등록한다. */
  devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
  register_chrdev_region(devno, 1, GPIO_DEVICE);

  /* 문자 디바이스를 위한 구조체를 초기화한다. */
  cdev_init(&gpio_cdev, &gpio_fops);

  gpio_cdev.owner = THIS_MODULE;
  count = 1;
  err = cdev_add(&gpio_cdev, devno, count); /* 문자 디바이스를 추가한다. */
  if (err < 0) {
    printk("Error : Device Add\n");
    return -1;
  }

  printk("'mknod /dev/%s c %d 0'\n", GPIO_DEVICE, GPIO_MAJOR);
  printk("'chmod 666 /dev/%s'\n", GPIO_DEVICE);

  /* GPIO 사용을 요청한다. */
  gpio_request(GPIO_LED, "LED");
  gpio_direction_output(GPIO_LED, 0);
  gpio_request(GPIO_SW, "SWITCH");
  gpio_direction_input(GPIO_SW);
  switch_irq = gpio_to_irq(GPIO_SW); /* GPIO 인터럽트 번호 획득 */
  err = request_irq(switch_irq, isr_func, IRQF_TRIGGER_RISING, "switch",
                    NULL); /* GPIO 인터럽트 핸들러 등록 */

  return 0;
}

void cleanup_module(void) {
  dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);

  mutex_destroy(&led_mutex); /* 뮤텍스를 해제한다. */
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

static int gpio_open(struct inode *inod, struct file *fil) {
  printk("GPIO Device opened(%d/%d)\n", imajor(inod), iminor(inod));

  return 0;
}

static int gpio_close(struct inode *inod, struct file *fil) {
  printk("GPIO Device closed(%d)\n",
         MAJOR(fil->f_path.dentry->d_inode->i_rdev));

  return 0;
}

static ssize_t gpio_read(struct file *inode, char *buff, size_t len,
                         loff_t *off) {
  int count;

  strcat(msg, " from Kernel");

  count = copy_to_user(buff, msg,
                       strlen(msg) + 1); /* 사용자 영역으로 데이터를 보낸다. */

  printk("GPIO Device(%d) read : %s(%d)\n",
         MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, count);

  return count;
}

static ssize_t gpio_write(struct file *inode, const char *buff, size_t len,
                          loff_t *off) {
  short count;
  char *cmd, *str;
  char *sep = ":";
  char *endptr, *pidstr;
  pid_t pid;
  memset(msg, 0, BLOCK_SIZE);
  count = copy_from_user(msg, buff,
                         len); /* 사용자 영역으로부터 데이터를 가져온다. */

  str = kstrdup(msg, GFP_KERNEL);
  cmd = strsep(&str, sep);
  pidstr = strsep(&str, sep);
  printk("Command : %s, Pid : %s\n", cmd, pidstr);
  cmd[1] = '\0';
  if (!strcmp(msg, "0")) {
    del_timer_sync(&timer); /* 타이머 삭제 */
    gpio_set_value(GPIO_LED, 0);
  } else {
    /* 타이머 초기화와 타이머 처리를 위한 함수 등록 */
    timer_setup(&timer, timer_func, 0);

    /* timer_list 구조체 초기화 : 주기 1초 */
    timer.expires = jiffies + (1 * HZ);

    add_timer(&timer); /* 타이머 추가 */
  }

  printk("GPIO Device(%d) write : %s(%ld)\n",
         MAJOR(inode->f_path.dentry->d_inode->i_rdev), msg, len);

  pid = simple_strtol(pidstr, &endptr, 10);
  if (endptr != NULL) {
    task = pid_task(find_vpid(pid), PIDTYPE_PID);
    if (task == NULL) {
      printk("Error : Can't find PID from user application\n");
      return 0;
    }
  }
  return count;
}