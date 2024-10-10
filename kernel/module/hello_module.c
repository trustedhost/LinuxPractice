#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL");

static int initModule(void) {
  printk(KERN_INFO "Hello, kernel module! \n");
  return 0;
}

static void cleanupModule(void) {
  printk(KERN_INFO "Good-bye, kernel-module! \n");
}

module_init(initModule);
module_exit(cleanupModule);
