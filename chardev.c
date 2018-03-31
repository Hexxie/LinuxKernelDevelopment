#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>

#include "chardev.h"

#define DEVICE_NAME "coffee"

#define BUF_LEN 80

static char message[BUF_LEN];
static char *messagePtr;

static int device_open(struct inode *inode,
                       struct file *file) {
  printk("device_open(%p)\n", file);

  return 0;
}

static int device_release(struct inode *inode,
                          struct file *file) {
  printk("device_release(%p, %p)\n", inode, file);

  return 0;
}

static ssize_t device_write(struct file *filp,
                            const char __user *buffer,
                            size_t count,
                            loff_t *offset) {
  
  printk("device write(%p, %s, %ld)", filp, buffer, count);
  struct scull_dev *dev = filp->private_data;
  struct scull_qset *dptr;
  int quantum = dev->quantum, qset = dev->qset;
  int itemsize = quantum * qset;
  int item, s_pos, q_pos, rest;

  ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

  if (down_interruptible(&dev->sem)) {
    return -ERESTARTSYS;
  }

  /* find listitem, qset index and offset in the quantum */
  item = (long)*f_pos / itemsize;
  rest = (long)*f_pos % itemsize;
  s_pos = rest / quantum; q_pos = rest % quantum;

  /* follow the list up to the right position */
  dptr = scull_follow(dev, item);
  if (dptr == NULL) {
    goto out;
  }

  if (!dptr->data) {
    dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);

    if (!dptr->data) {
     goto out;
    }

    memset(dptr->data, 0, qset * sizeof(char *));
  }
  if (!dptr->data[s_pos]) {
    dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
    if (!dptr->data[s_pos]) {
      goto out;
    }
  }
  /* write only up to the end of this quantum */
  if (count > quantum - q_pos) {
    count = quantum - q_pos;
  }
  if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
    retval = -EFAULT;
    goto out;
  }

  *f_pos += count;
  retval = count;
  /* update the size */
  if (dev->size < *f_pos) {
    dev->size = *f_pos;
  }

  out:
  up(&dev->sem);
  return retval;
}

static ssize_t device_read(struct file *filp,
			   char __user *buffer,
 			   size_t count,
			   loff_t *offset) {
  int bytes_read = 0;
  printk("device_read(%p,%p,%d)\n", filp, buffer, count);
  if(*messagePtr == 0) {
    return 0;
  }
 
  struct scull_dev *dev = filp->private_data;
  struct scull_qset *dptr;
  int quantum = dev->quantum, qset = dev->qset;
  int itemsize = quantum * qset; //count bites in the listitem
  int item, s_pos, q_pos, rest;
  ssize_t retval = 0;

  if (down_interruptible(&dev->sem)) {
    return -ERESTARTSYS;
  }
  if(*f_pos >= dev->size) {
    goto out; //!!!TBD!!!
  }  
  if(*f_pos + count > dev->size) {
    count = dev->size - *f_pos;
  }

  //find listitem, qset index and offset in quantum
  item = (long)*f_pos / itemsize;
  rest = (long)*f_pos % itemsize;
  s_pos = rest / quantum; q_pos = rest % quantum;
  /* follow the list up to the right position (defined elsewhere) */
  dptr = scull_follow(dev, item);
  if (dptr = = NULL || !dptr->data || ! dptr->data[s_pos]) {
    goto out; /* don't fill holes */
  } 
  /* read only up to the end of this quantum */
  if (count > quantum - q_pos) {
    count = quantum - q_pos;
  }
  if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
    retval = -EFAULT;
    goto out;
  }
  *f_pos += count;
  retval = count;
  out:
  up(&dev->sem);
  return retval;
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
  NULL,   /* seek */
  device_read, 
  device_write,
  NULL,   /* readdir */
  NULL,   /* select */
  device_ioctl,   /* ioctl */
  NULL,   /* mmap */
  device_open,
  NULL,  /* flush */
  device_release  /* a.k.a. close */
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

