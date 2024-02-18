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
#include <linux/timer.h>
#include <linux/semaphore.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>

#define TIMER_CNT   1
#define TIMER_NAME   "timer"
#define LEDOFF 0
#define LEDON  1

#define close_cmd    (_IO(0xEF, 0x1))
#define open_cmd     (_IO(0xEF, 0x2))
#define setpriod_cmd (_IO(0xEF, 0x3))

/* gpio 结构体 */
struct timer_dev
{
    dev_t devid; 
    int major;
    int minor;

    struct cdev cdev;

    struct class *class;
    struct device *device;

    struct device_node *nd;
    int led_gpio;

    int timerpriod;
    struct timer_list timer;
    spinlock_t lock;

};
struct timer_dev timerdev;

static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &timerdev;

    timerdev.timerpriod = 1000;
    
    return 0;
}

static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timerpriod;
    unsigned long flags;

    switch(cmd)
    {
        case close_cmd:
            del_timer_sync(&dev->timer);
            break;

        case open_cmd:
            spin_lock_irqsave(&dev->lock, flags);
            timerpriod = dev->timerpriod;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerpriod));
            break;

        case setpriod_cmd:
            spin_lock_irqsave(&dev->lock, flags);
            dev->timerpriod = arg;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
            break;
        default:
            break;

    }

    return 0;
}
static const struct file_operations led_fops = {
    .owner		=	THIS_MODULE,
	.open		=	led_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
	
};

void timer_function(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)arg;
    static int sta = 1;
    int timerpriod;
    unsigned long flags;

    sta = !sta;
    gpio_set_value(dev->led_gpio, sta);
    spin_lock_irqsave(&dev->lock, flags);
    timerpriod = dev->timerpriod;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerpriod));

}

static int __init timer_init(void)
{
    int ret = 0;

    /* 初始化设备号*/
  
    if(timerdev.major)
    {
        timerdev.devid = MKDEV(timerdev.major,0);
        register_chrdev_region(timerdev.devid, TIMER_CNT,TIMER_NAME);
        printk("define major and minor\r\n");
    }
    else
    {
        alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT,TIMER_NAME);
        timerdev.major = MAJOR(timerdev.devid);
        timerdev.minor = MINOR(timerdev.devid);
        printk("not define major and minor\r\n");
    }
    printk("timerdev major = %d,minor = %d\n", timerdev.major, timerdev.minor);
    /* 初始化 cdev*/
    timerdev.cdev.owner = THIS_MODULE;
    cdev_init(&timerdev.cdev, &led_fops);

    cdev_add(&timerdev.cdev, timerdev.devid ,TIMER_CNT);

    timerdev.class = class_create(THIS_MODULE,TIMER_NAME);
    if (IS_ERR(timerdev.class)) {
		return PTR_ERR(timerdev.class);
	}

    timerdev.device = device_create(timerdev.class,NULL,timerdev.devid,NULL,TIMER_NAME);
    if (IS_ERR(timerdev.device)) {
		return PTR_ERR(timerdev.device);
	}


    /* 设备树相关 */
    timerdev.nd = of_find_node_by_path("/gpioled");
    if(timerdev.nd == NULL) {
        printk("timerdev node not found\r\n");
        return -EINVAL;
    }else
    {
        printk("timerdev node found\r\n");
    }
    
    timerdev.led_gpio = of_get_named_gpio(timerdev.nd, "led-gpio",0);
    if(timerdev.led_gpio < 0) 
    {
        printk("cannot get gpio name\r\n");
        return -EINVAL;
    }
    printk("led-gpio num = %d\r\n", timerdev.led_gpio);

    ret = gpio_direction_output(timerdev.led_gpio, 1);
    if(ret < 0) {
        printk("can't set gpio\r\n");
    }

    init_timer(&timerdev.timer);
    timerdev.timer.function = timer_function;
    timerdev.timer.data = (unsigned long)&timerdev;

    return 0;


}

static void __exit timer_exit(void)
{
    gpio_set_value(timerdev.led_gpio, 1);
    del_timer_sync(&timerdev.timer);

    cdev_del(&timerdev.cdev);
    unregister_chrdev_region(timerdev.devid, TIMER_CNT);
    device_destroy(timerdev.class, timerdev.devid);
    class_destroy(timerdev.class);

}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");

