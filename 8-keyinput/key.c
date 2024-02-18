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

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>

#define KEY_CNT   1
#define KEY_NAME   "key"
#define KEY0VALUE  0xf0
#define INVAKEY    0x00



/* gpio 结构体 */
struct key_dev
{
    dev_t devid; 
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int key_gpio;
    atomic_t keyvalue;
};
struct key_dev keydev;

static int keyio_init(void)
{
/* 设备树相关 */
    keydev.nd = of_find_node_by_path("/key");
    if(keydev.nd == NULL) {
        printk("key node not found\r\n");
        return -EINVAL;
    }else
    {
        printk("key node found\r\n");
    }
    
    keydev.key_gpio = of_get_named_gpio(keydev.nd, "key-gpio",0);
    if(keydev.key_gpio < 0) 
    {
        printk("cannot get gpio name\r\n");
        return -EINVAL;
    }
    printk("key-gpio num = %d\r\n", keydev.key_gpio);

    gpio_request(keydev.key_gpio,"key0");
    gpio_direction_input(keydev.key_gpio);

    return 0;
}
static int led_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    filp->private_data = &keydev;

    ret = keyio_init();
    if(ret < 0)
    {
        return ret;
    }
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf,
                        size_t cnt, loff_t *offt)
{
    int ret = 0;
    unsigned char value;
    struct key_dev *dev = filp->private_data;

    if(gpio_get_value(dev->key_gpio) == 0) //key按下
    {
        while(!gpio_get_value(dev->key_gpio));
        atomic_set(&dev->keyvalue,KEY0VALUE);
    }
    else{
        atomic_set(&dev->keyvalue,INVAKEY);
    }

    value = atomic_read(&dev->keyvalue);
    ret = copy_to_user(buf,&value, sizeof(value));  

    return ret;
}

// static int led_release(struct inode *inode, struct file *filp)
// {
    
//     return 0;
// }

// static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
// {
// 	int retvalue;
// 	unsigned char databuf[1];
// 	unsigned char ledstat;
//     struct keydev_dev *dev = filp->private_data;

// 	retvalue = copy_from_user(databuf, buf, cnt);
// 	if(retvalue < 0) {
// 		printk("kernel write failed!\r\n");
// 		return -EFAULT;
// 	}

// 	ledstat = databuf[0];		/* 获取状态值 */

// 	if(ledstat == LEDON) {	
// 		gpio_set_value(dev->key_gpio, 0);		/* 打开LED灯 */
// 	} else if(ledstat == LEDOFF) {
// 		gpio_set_value(dev->key_gpio, 1);	/* 关闭LED灯 */
// 	}
// 	return 0;
// }

static const struct file_operations key_fops = {
    .owner		=	THIS_MODULE,
	.open		=	led_open,
    .read       =	led_read,
};


static int __init mykey_init(void)
{


    atomic_set(&keydev.keyvalue, INVAKEY);

    /* 初始化设备号*/

    if(keydev.major)
    {
        keydev.devid = MKDEV(keydev.major,0);
        register_chrdev_region(keydev.devid, KEY_CNT,KEY_NAME);
    }
    else
    {
        alloc_chrdev_region(&keydev.devid, 0, KEY_CNT,KEY_NAME);
        keydev.major = MAJOR(keydev.devid);
        keydev.minor = MINOR(keydev.devid);
    }
    printk("major = %d\r\nminor = %d\r\n", keydev.major, keydev.minor);
    /* 初始化 cdev*/
    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev, &key_fops);

    cdev_add(&keydev.cdev, keydev.devid ,KEY_CNT);

    keydev.class = class_create(THIS_MODULE,KEY_NAME);
    if (IS_ERR(keydev.class)) {
		return PTR_ERR(keydev.class);
	}

    keydev.device = device_create(keydev.class,NULL,keydev.devid,NULL,KEY_NAME);
    if (IS_ERR(keydev.device)) {
		return PTR_ERR(keydev.device);
	}


    return 0;


}

static void __exit mykey_exit(void)
{

    cdev_del(&keydev.cdev);
    unregister_chrdev_region(keydev.devid, KEY_CNT);
    device_destroy(keydev.class, keydev.devid);
    class_destroy(keydev.class);

}

module_init(mykey_init);
module_exit(mykey_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MOOSA");

