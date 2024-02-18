#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define ATOMIC_CNT  1
#define ATOMIC_NAME "atomic"
#define LEDOFF 0
#define LEDON 1

struct atomic_device
{
    int major;
    int minor;

    struct class *class;
    struct device *device;

    dev_t devid;
    struct cdev cdev;

    struct device_node *nd;
    int led_gpio;

    atomic_t lock;

};

struct atomic_device atomicdev;


static int atomic_open(struct inode *inode, struct file *filp)
{
    if(! atomic_dec_and_test(&atomicdev.lock))
    {
        atomic_inc(&atomicdev.lock);
        return -EBUSY;
    }
    filp->private_data = &atomicdev;
    return 0;
}

static ssize_t atmic_read(struct file *filp,  char __user *buf, 
                            size_t cnt, loff_t *offt)
{

    return 0;
}

static ssize_t atomic_write(struct file *filp, const char __user *buf,
                            size_t cnt, loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct atomic_device *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0) {
    printk("kernel write failed!\r\n");
    return -EFAULT;
    }

    ledstat = databuf[0]; 

    if(ledstat == LEDON) { 
    gpio_set_value(dev->led_gpio, 0);
    } 
    else if(ledstat == LEDOFF) { 
    gpio_set_value(dev->led_gpio, 1); 
    } 

    return 0;
}


static int atomic_release(struct inode *inode, struct file *filp)
{
    struct atomic_device *dev = filp->private_data;

    atomic_inc(&dev->lock);

    return 0;

}

static const struct file_operations atomic_fops = {
	.owner =     THIS_MODULE,
    .open =	     atomic_open,
    .read =	     atmic_read,
    .write =     atomic_write,
	.release =   atomic_release,
};


static int __init led_init(void)
{
    int ret = 0;

    atomic_set(&atomicdev.lock, 1);

    atomicdev.nd = of_find_node_by_path("/gpioled");
    if(atomicdev.nd == NULL)
    {
        printk("Couldn't find led node\r\n");
        return -EINVAL;
    }
    else
    {
        printk("Found led node\r\n");
    }

    atomicdev.led_gpio = of_get_named_gpio(atomicdev.nd, "led-gpio", 0);
    if( atomicdev.led_gpio < 0)
    {
        printk("Couldn't get led gpio\r\n");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", atomicdev.led_gpio);

    ret = gpio_direction_output(atomicdev.led_gpio, 1);
    if (ret < 0)
    {
        printk("led-gpio output error! \r\n");
    }

    if(atomicdev.major)
    {
        atomicdev.devid = MKDEV(atomicdev.major, 0);
        register_chrdev_region(atomicdev.devid , ATOMIC_CNT, ATOMIC_NAME);
    }
    else 
    {
        alloc_chrdev_region(&atomicdev.devid, 0, ATOMIC_CNT, ATOMIC_NAME);
        atomicdev.major = MAJOR(atomicdev.devid);
        atomicdev.minor = MINOR(atomicdev.devid);
    }
    printk("atomic major = %d \r\natomic minor = %d \r\n", atomicdev.major, atomicdev.minor);


    atomicdev.cdev.owner = THIS_MODULE;
    cdev_init(&atomicdev.cdev, &atomic_fops);
    cdev_add(&atomicdev.cdev, atomicdev.devid, ATOMIC_CNT);

    atomicdev.class = class_create(THIS_MODULE, ATOMIC_NAME);
    if(IS_ERR(atomicdev.class))
    {
        return PTR_ERR(atomicdev.class);
    }

    atomicdev.device = device_create(atomicdev.class, NULL, atomicdev.devid, NULL, ATOMIC_NAME);
    if(IS_ERR(atomicdev.device))
    {
        return PTR_ERR(atomicdev.device);
    }

    return 0;
}

static void __exit led_exit(void)
{

    cdev_del(&atomicdev.cdev);
    unregister_chrdev_region(atomicdev.devid, ATOMIC_CNT);

    device_destroy(atomicdev.class, atomicdev.devid);
    class_destroy(atomicdev.class);


}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");

