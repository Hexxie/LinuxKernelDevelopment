#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>

#include "chardev.h"

#define DEVICE_NAME "coffee"

#define BUF_LEN 80

static atomic_t deviceOpen = ATOMIC_INIT(0);
static char message[BUF_LEN];
static char *messagePtr;

static int device_open(struct inode *inode,
                       struct file *file) {
  printk("device_open(%p)\n", file);

  if(atomic_read(&deviceOpen)) {
    return -EBUSY;
  }

  atomic_inc(&deviceOpen);
  messagePtr = message;

  return 0;
}

static int device_release(struct inode *inode,
                          struct file *file) {
  printk("device_release(%p, %p)\n", inode, file);

  atomic_dec(&deviceOpen);

  return 0;
}

static ssize_t device_write(struct file *file,
                            const char *buffer,
                            size_t length,
                            loff_t *offset) {
  int i;
  printk("device write(%p, %s, %zu)", file, buffer, length);
  for(i = 0; i < length && BUF_LEN; i++) {
    get_user(message[i], buffer+i);  
  }
  messagePtr = message;
  return i;
}

static ssize_t device_read(struct file *file,
			   char *buffer,
 			   size_t length,
			   loff_t *offset) {
  int bytes_read = 0;
  printk("device_read(%p,%p,%zu)\n", file, buffer, length);
  if(*messagePtr == 0) {
    return 0;
  }
 
  while(length && *messagePtr) {
    put_user(*(messagePtr++), buffer++);
    length--;
    bytes_read++;
  }
  printk ("Read %d bytes, %zu left\n", bytes_read, length);
  return bytes_read;
}

int device_ioctl(struct inode *inode,
                 struct file *file,
                 unsigned int ioctl_num,
                 unsigned long ioctl_param) {
  int i;
  char *temp;
  char ch;
  switch (ioctl_num) {

    case IOCTL_SET_MSG:
      temp = (char *) ioctl_param;
      //get value from user space
      get_user(ch, temp);
      device_write(file, (char*) ioctl_param, i);
    break;

    case IOCTL_GET_MSG:
      i = device_read(file, (char *) ioctl_param, 99, 0); 
      //write value to user space
      put_user('\0', (char *) ioctl_param+i);
      break;
  }

  return 0;
}


/***************** module declarations **********************/

struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = device_read,
  .write = device_write,
  .ioctl = device_ioctl,
  .open = device_open,
  .release = device_release
};

/* Initialize the module - Register the character device */
int init_module()
{
  int ret_val;

  /* Register the character device (atleast try) */
  ret_val = module_register_chrdev(MAJOR_NUM, 
                                 DEVICE_NAME,
                                 &Fops);

  /* Negative values signify an error */
  if (ret_val < 0) {
    printk ("%s failed with %d\n",
            "Sorry, registering the character device ",
            ret_val);
    return ret_val;
  }

  printk ("%s The major device number is %d.\n",
          "Registeration is a success", 
          MAJOR_NUM);
  printk ("If you want to talk to the device driver,\n");
  printk ("you'll have to create a device file. \n");
  printk ("We suggest you use:\n");
  printk ("mknod %s c %d 0\n", DEVICE_FILE_NAME, 
          MAJOR_NUM);
  printk ("The device file name is important, because\n");
  printk ("the ioctl program assumes that's the\n");
  printk ("file you'll use.\n");

  return 0;
}


/* Cleanup - unregister the appropriate file from /proc */
void cleanup_module()
{
  int ret;

  /* Unregister the device */
  ret = module_unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
 
  /* If there's an error, report it */ 
  if (ret < 0)
    printk("Error in module_unregister_chrdev: %d\n", ret);
}  

