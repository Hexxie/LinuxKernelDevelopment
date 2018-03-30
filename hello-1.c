#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void) {
  printk("<1>Hello world 1.\n");
  return 10;
}

void cleanup_module(void) {
  printk(KERN_ALERT "Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");
