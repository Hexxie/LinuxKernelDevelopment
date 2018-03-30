#define MODULE
#define LINUX
#define __KERNEL__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int hello_module_init(void) {
  printk("<1>Hello world 1.\n");
  return 10;
}

void hello_module_exit(void) {
  printk(KERN_ALERT "Goodbye world 1.\n");
}

module_init(hello_module_init);
module_exit(hello_module_exit);

MODULE_LICENSE("GPL");
