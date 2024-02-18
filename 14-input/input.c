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
#include <linux/input.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>

#define INPUT_CNT    1
#define INPUT_NAME   "keyinput"
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
    struct input_dev *inputdev;
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
        input_report_key(dev->inputdev, keydesc->value, 1);
        input_sync(dev->inputdev);
    } 
    else
    {
        input_report_key(dev->inputdev, keydesc->value, 0);
        input_sync(dev->inputdev);
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
    
    irqdev.irqkeydesc.name[10] = "key0";
    
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

    irqdev.inputdev = input_allocate_device();
    irqdev.inputdev->name = INPUT_NAME;

    __set_bit(EV_KEY, irqdev.inputdev->evbit); /*按键事件 */
    __set_bit(EV_REP, irqdev.inputdev->evbit); /* 重复事件 */
    __set_bit(KEY_0, irqdev.inputdev->keybit);

    ret = input_register_device(irqdev.inputdev);
    if (ret) 
    {
        printk("register input device failed!\r\n"); 
        return ret;
    }

    return 0;
}


static int __init mykey_init(void)
{
    
    keyio_init();

    return 0;
}

static void __exit mykey_exit(void)
{
    del_timer_sync(&irqdev.timer);
    free_irq(irqdev.irqkeydesc.irqnum, &irqdev);

    input_unregister_device(irqdev.inputdev);
    input_free_device(irqdev.inputdev);

}

module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");
