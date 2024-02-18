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
#include <linux/wait.h>

#define IRQ_CNT    1
#define IRQ_NAME   "blockio"
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

    struct timer_list timer;
    struct irq_keydesc irqkeydesc;
    
    wait_queue_head_t waitheader;
};
struct irq_dev irqdev;

static irqreturn_t key0_handler(int irq, void *dev_id)
{
    struct irq_dev *dev = (struct irq_dev *)dev_id;

    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);

}

void timer_function(unsigned long arg)
{
    unsigned char value;
    struct irq_dev *dev=(struct irq_dev *)arg;
    struct irq_keydesc *keydesc;

    keydesc = &dev->irqkeydesc;
    value = gpio_get_value(keydesc->gpio);
    if(value == 0)
    {
         printk("key press *****\r\n");
          atomic_set(&dev->keyvalue, keydesc->value);
    }
    else{
        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey, 1);
         printk("key press #######\r\n");
    }

    if(atomic_read(&dev->releasekey))
    {
        wake_up_interruptible(&dev->waitheader);
        printk("wake_up_interruptible\r\n");
    }

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
    
    // irqdev.irqkeydesc.name[10] = "key0";
    strcpy(irqdev.irqkeydesc.name,"key0");
    
    gpio_request(irqdev.irqkeydesc.gpio,irqdev.irqkeydesc.name);
    gpio_direction_input(irqdev.irqkeydesc.gpio);

    irqdev.irqkeydesc.irqnum = irq_of_parse_and_map(irqdev.nd, 0);
    irqdev.irqkeydesc.handler = key0_handler;
    irqdev.irqkeydesc.value = KEY0VALUE;

    ret = request_irq(irqdev.irqkeydesc.irqnum, 
                      irqdev.irqkeydesc.handler,
                      IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
	                  irqdev.irqkeydesc.name, 
                      &irqdev);
    if(ret < 0)
    {
        printk("irq %d request failed\r\n", irqdev.irqkeydesc.irqnum);
        return -EFAULT;
    }

    init_timer(&irqdev.timer);
    irqdev.timer.function = timer_function;

    init_waitqueue_head(&irqdev.waitheader);


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


    DECLARE_WAITQUEUE(wait,current);
    if(atomic_read(&dev->releasekey) == 0)
    {
        add_wait_queue(&dev->waitheader, &wait);
        __set_current_state(TASK_INTERRUPTIBLE);
        schedule();

        if(signal_pending(current))
        {
            ret = -ERESTARTSYS;
            goto wait_error;
        }
        __set_current_state(TASK_RUNNING);
        remove_wait_queue(&dev->waitheader,&wait);
    }




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

wait_error:
    set_current_state(TASK_RUNNING);
    remove_wait_queue(&dev->waitheader,&wait);
    return ret;

}


static const struct file_operations irq_fops = {
    .owner		=	THIS_MODULE,
	.open		=	led_open,
    .read       =	led_read,
};


static int __init mykey_init(void)
{
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

    atomic_set(&irqdev.keyvalue, INVALUE);
    atomic_set(&irqdev.releasekey, 0);
    
    keyio_init();

    /*初始化wait header*/
    // init_waitqueue_head(&irqdev.waitheader);

    return 0;


}

static void __exit mykey_exit(void)
{
    del_timer_sync(&irqdev.timer);
    free_irq(irqdev.irqkeydesc.irqnum, &irqdev);

    cdev_del(&irqdev.cdev);
    unregister_chrdev_region(irqdev.devid, IRQ_CNT);
    device_destroy(irqdev.class, irqdev.devid);
    class_destroy(irqdev.class);

}

module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");

