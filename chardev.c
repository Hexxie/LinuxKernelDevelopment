#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h> 

#include <linux/fs.h>
#include <linux/cdev.h>

#include "chardev.h"

MODULE_LICENSE("GPL");

/************************** Global Data Definitions **************************/

#define DEVICE_NAME "coffee"
#define BUF_LEN 80
#define  CLASS_NAME  "chardrv"

/************************* Static Function Prototypes ************************/
static int device_open(struct inode *inode,
                       struct file *file);
static int device_release(struct inode *inode,
                          struct file *file);

static ssize_t device_write(struct file *filp,
                            const char __user *buffer,
                            size_t count,
                            loff_t *f_pos);

static ssize_t device_read(struct file *filp,
			   char __user *buffer,
 			   size_t count,
			   loff_t *f_pos);

static ssize_t device_read_old(struct inode *inode,
                               struct file *file,
                               char *buffer,
                               size_t length,
                               loff_t *offset);

/************************** Static Data Definitions **************************/

static int majorNumber;         
static char message[BUF_LEN];
static short size_of_message;
static atomic_t isDeviceOpen = ATOMIC_INIT(0);
static struct class*  deviceClass  = NULL; 
static struct device* coffeeDevice = NULL;

static struct file_operations fops = {
  .owner = THIS_MODULE,
  .write = device_write,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
  .read = device_read_old,
#else
  .read = device_read,
#endif
  .open = device_open,
  .release = device_release
};

/********************************** Functions ********************************/

/** @brief initialization function
 *  Dynamically allocate major number for required device.
 *  @return 0 in case of success
 */
int init_module(void)
{
  int ret_val;

  printk("Initialization lkm");

  //dynamically allocate a major number fot the device
  majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
  if(majorNumber < 0) {
    printk(KERN_ALERT "Failed to register major number");
    return majorNumber;
  }
  printk("Registered with major number: %d", majorNumber);

   // Register the device class
   deviceClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(deviceClass)){                
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk("Failed to register device class\n");
      return PTR_ERR(deviceClass);          
   }
   printk("EBBChar: device class registered correctly\n");
 
   // Register the device driver
   coffeeDevice = device_create(deviceClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(coffeeDevice)){             
      class_destroy(deviceClass);           
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk("Failed to create the device\n");
      return PTR_ERR(coffeeDevice);
   }
   printk("EBBChar: device class created correctly\n");

  return 0;
}


/** @brief cleanup function
 *  Unregister used device.
 */
void cleanup_module()
{
  int ret;

  /* Unregister the device */
  unregister_chrdev(majorNumber, DEVICE_NAME);
}  

/** @brief open function which called everytime when device is opened
 *  Increment counter if device was not opened, or return EBUSY if counter
 *  already incremented
 *  @param inode - pointer to an inode object (see linux/fs.h)
 *  @param file  - ponter to a file object (see linux/fs.h)
 *  return 0 in case of success or BUSY if counter incremented
 */
static int device_open(struct inode *inode,
                       struct file *file) {

  printk("device_open(%p)\n", file);
  if(atomic_read(&isDeviceOpen)) {
    return -EBUSY;
  }
  atomic_inc(&isDeviceOpen);	 
  return 0;
}

/** @brief release function which called everytime when device is  closed
 *  Decrement counter only.
 *  @param inode - pointer to an inode object (see linux/fs.h)
 *  @param file  - ponter to a file object (see linux/fs.h)
 *  return 0 in case of success
 */
static int device_release(struct inode *inode,
                          struct file *file) {
  printk("device_release(%p, %p)\n", inode, file);
  atomic_dec(&isDeviceOpen);
  return 0;
}

/** @brief function to write symbols from user space to kernel
 *  @param filp  - ponter to a file object (see linux/fs.h)
 *  @param buffer - buffer with strick which will be written to the device
 *  @param len - length of the array with data
 *  @param f_pos - offces if required
 *  return 0 in case of success
 */
static ssize_t device_write(struct file *filp,
                            const char __user *buffer,
                            size_t len,
                            loff_t *f_pos) {
  
  printk("device write(%p, %s, %ld)", filp, buffer, len);
  sprintf(message, "%s(%zu letters)", buffer, len);
  size_of_message = strlen(message);
  printk(KERN_INFO "Received %zu characters from the user\n", len);
}

/** @brief function to read symbols from kernel to user space
 *  @param filp  - ponter to a file object (see linux/fs.h)
 *  @param buffer - buffer with strick which will be written to the device
 *  @param len - length of the array with data
 *  @param f_pos - offces if required
 *  return 0 in case of success
 */
static ssize_t device_read(struct file *filp,
			   char __user *buffer,
 			   size_t len,
			   loff_t *f_pos) {
  int result = 0;
  result = copy_to_user(buffer, message, size_of_message);
  if(result == 0) {
    printk(KERN_INFO "Sent %d characters to the user\n", size_of_message);
    return (size_of_message=0);  
  } else {
    printk(KERN_INFO "failed to send character to the user. Error: %d", result);
    return -EFAULT;
  }
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))

/***************** compatibility routines *******************/

static ssize_t device_read_old(struct inode *inode,
                               struct file *file,
                               char *buffer,
                               size_t length,
                               loff_t *offset)
{
  return device_read(file, buffer, length, offset);
}

#endif

