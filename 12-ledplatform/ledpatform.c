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
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>


#define LED_CNT  1		  	/* 设备号个数 */
#define LED_NAME  "ledpaltform"	/* 名字 */
#define LEDOFF  0			/* 关灯 */
#define LEDON   1			/* 开灯 */
 


/* dtsled设备结构体 */
struct dtsled_dev{
	dev_t devid;			/* 设备号 	 */
	struct cdev cdev;		/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;	/* 设备 	 */
	int major;				/* 主设备号	  */
	int minor;				/* 次设备号   */
	struct device_node *nd;
	int led0;
};

struct dtsled_dev dtsled;	/* led设备 */


void led_switch(u8 sta)
{
	if(sta == LEDON) {
		gpio_set_value(dtsled.led0, 0);
	}else if(sta == LEDOFF) {
		gpio_set_value(dtsled.led0, 1);
	}	
}

static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dtsled; /* 设置私有数据 */
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
	int retvalue;
	unsigned char databuf[1];
	unsigned char ledstat;

	retvalue = copy_from_user(databuf, buf, cnt);
	if(retvalue < 0) {
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	ledstat = databuf[0];		/* 获取状态值 */

	if(ledstat == LEDON) {	
		led_switch(LEDON);		/* 打开LED灯 */
	} else if(ledstat == LEDOFF) {
		led_switch(LEDOFF);	/* 关闭LED灯 */
	}
	return 0;
}


/* 设备操作函数 */
static struct file_operations dtsled_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
};


static int led_probe(struct platform_device *dev)
{
	printk("led driver and device was matached!\r\n");

	if (dtsled.major) {		/*  定义了设备号 */
		dtsled.devid = MKDEV(dtsled.major, 0);
		register_chrdev_region(dtsled.devid, LED_CNT, LED_NAME);
	} else {						/* 没有定义设备号 */
		alloc_chrdev_region(&dtsled.devid, 0, LED_CNT, LED_NAME);	/* 申请设备号 */
		dtsled.major = MAJOR(dtsled.devid);	/* 获取分配号的主设备号 */
		//dtsled.minor = MINOR(dtsled.devid);	/* 获取分配号的次设备号 */
	}
	printk("newcheled major=%d\r\nminor=%d\r\n",dtsled.major, dtsled.minor);

	/* 2、初始化cdev */
	cdev_init(&dtsled.cdev, &dtsled_fops);
	
	/* 3、添加一个cdev */
	cdev_add(&dtsled.cdev, dtsled.devid, LED_CNT);

	/* 4、创建类 */
	dtsled.class = class_create(THIS_MODULE, LED_NAME);
	if (IS_ERR(dtsled.class)) {
		return PTR_ERR(dtsled.class);
	}

	/* 5、创建设备 */
	dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, LED_NAME);
	if (IS_ERR(dtsled.device)) {
		return PTR_ERR(dtsled.device);
	}

	dtsled.nd = of_find_node_by_path("/gpioled");
	if(dtsled.nd == NULL)
	{
		printk("gpioled node nost find!\r\n");
		return -EINVAL;
	}

	dtsled.led0 = of_get_named_gpio(dtsled.nd, "led-gpio", 0);
	if(dtsled.led0 < 0) 
	{
		printk("can't get led-gpio\r\n");
		return -EINVAL;
	}
	gpio_request(dtsled.led0, "led0");
	gpio_direction_output(dtsled.led0, 1); /*设置为输出，默认高电平 */

	return 0;
}

static int led_remove(struct platform_device *dev)
{
	gpio_set_value(dtsled.led0, 1);

	cdev_del(&dtsled.cdev);
	unregister_chrdev_region(dtsled.devid, LED_CNT);
	device_destroy(dtsled.class, dtsled.devid);
	class_destroy(dtsled.class);

	return 0;
}

static struct of_device_id led_of_match[] = 
{
	{ .compatible = "atkalpha-gpioled"},
	{ }
};

static struct platform_driver led_driver = 
{
	.driver = 
		{
			.name = "imx6ul-led",
			.of_match_table = led_of_match,
		},
	.probe = led_probe,
	.remove = led_remove,
};

static int __init leddriver_init(void)
{
	return platform_driver_register(&led_driver);
}

static void __exit leddriver_exit(void)
{
	return platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");