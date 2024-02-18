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
#include <linux/irq.h>
#include <linux/of_irq.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>

#define IRQ_CNT    1
#define IRQ_NAME   "imxirq"
#define KEY0VALUE  0xf0
#define INVALUE    0x00

struct irq_keydesc
{
    int gpio;
    int irqnum;
    unsigned char value;
    char name[10];
    irqreturn_t (*handler)(int, void *);
};

/* gpio 结构体 */
struct irq_dev
{
    dev_t devid; 
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;

    atomic_t keyvalue;
    atomic_t releasekey;

    struct irq_keydesc irqkeydesc;
    unsigned char curkeynum;
};
struct irq_dev irqdev;

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    // struct irq_dev *dev = (struct irq_dev *)dev_id;

    printk("enter irq : %d\r\n",irq);
    return IRQ_RETVAL(IRQ_HANDLED);

}



static int keyio_init(void)
{
    int ret = 0;

/* 设备树相关 */
    irqdev.nd = of_find_node_by_path("/key");
    if(irqdev.nd == NULL) {
        printk("key node not found\r\n");
        return -EINVAL;
    }
    else
        printk("key node found\r\n");
    
 
    irqdev.irqkeydesc.gpio = of_get_named_gpio(irqdev.nd, "key-gpio",0);
    if(irqdev.irqkeydesc.gpio < 0) 
    {
        printk("cannot get key gpio\r\n");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", irqdev.irqkeydesc.gpio);
    
    
    
    gpio_request(irqdev.irqkeydesc.gpio,irqdev.irqkeydesc.name);
    gpio_direction_input(irqdev.irqkeydesc.gpio);

    /*初始化IRQ中断信息*/
    irqdev.irqkeydesc.irqnum = irq_of_parse_and_map(irqdev.nd, 0);
    irqdev.irqkeydesc.handler = key0_handler;
    irqdev.irqkeydesc.value = KEY0VALUE;
  

    ret = request_irq(irqdev.irqkeydesc.irqnum, 
                      irqdev.irqkeydesc.handler,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	                  "key0", 
                      &irqdev);
    if(ret < 0)
    {
        printk("irq %d request failed\r\n", irqdev.irqkeydesc.irqnum);
        return -EFAULT;
    }
    return 0;
}
static int led_open(struct inode *inode, struct file *filp)
{

    filp->private_data = &irqdev;

    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf,
                        size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char keyvalue = 0;
    unsigned char releasekey = 0;
    struct irq_dev *dev = filp->private_data;

    keyvalue = atomic_read(&dev->keyvalue);
    releasekey = atomic_read(&dev->releasekey);

    if(releasekey)
    {
        if(keyvalue & 0x80)
        {
            keyvalue &= ~0x80;
            ret = copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }
        else 
        {
            return -EINVAL;
        }
        atomic_set(&dev->releasekey, 0);
    }
    else
        return -EINVAL;

    return 0;
}


static const struct file_operations irq_fops = {
    .owner		=	THIS_MODULE,
	.open		=	led_open,
    .read       =	led_read,
};


static int __init mykey_init(void)
{


    atomic_set(&irqdev.keyvalue, INVALUE);

    /* 初始化设备号*/

    if(irqdev.major)
    {
        irqdev.devid = MKDEV(irqdev.major,0);
        register_chrdev_region(irqdev.devid, IRQ_CNT,IRQ_NAME);
    }
    else
    {
        alloc_chrdev_region(&irqdev.devid, 0, IRQ_CNT,IRQ_NAME);
        irqdev.major = MAJOR(irqdev.devid);
        irqdev.minor = MINOR(irqdev.devid);
    }
    printk("major = %d\r\nminor = %d\r\n", irqdev.major, irqdev.minor);
    /* 初始化 cdev*/
    irqdev.cdev.owner = THIS_MODULE;
    cdev_init(&irqdev.cdev, &irq_fops);

    cdev_add(&irqdev.cdev, irqdev.devid ,IRQ_CNT);

    irqdev.class = class_create(THIS_MODULE,IRQ_NAME);
    if (IS_ERR(irqdev.class)) {
		return PTR_ERR(irqdev.class);
	}

    irqdev.device = device_create(irqdev.class,NULL,irqdev.devid,NULL,IRQ_NAME);
    if (IS_ERR(irqdev.device)) {
		return PTR_ERR(irqdev.device);
	}

   keyio_init();

    return 0;


}

static void __exit mykey_exit(void)
{
    free_irq(irqdev.irqkeydesc.gpio,&irqdev);

    cdev_del(&irqdev.cdev);
    unregister_chrdev_region(irqdev.devid, IRQ_CNT);
    device_destroy(irqdev.class, irqdev.devid);
    class_destroy(irqdev.class);

}

module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");

