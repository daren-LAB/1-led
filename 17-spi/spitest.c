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
#include <linux/spi/spi.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>
#include "w25xx.h"

#define WQ_CNT 1
#define WQ_NAME "wq25"
#define WRITE_REG2       0x20 
#define TX_ADDR         0x10  

struct wq_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;
    int cs_gpio;
};
struct wq_dev wqdev;

static int w25q80_read_regs(struct wq_dev *dev,void *buf,int len)
{
    int ret = 0;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev->cs_gpio, 0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

    t->rx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    kfree(t);
    gpio_set_value(dev->cs_gpio,1);
    return ret;
}

static int w25q80_write_regs(struct wq_dev *dev ,void *buf,int len)
{
    int ret = 0;
    struct spi_message m;
    struct spi_transfer *t;
    struct spi_device *spi = (struct spi_device *)dev->private_data;

    gpio_set_value(dev->cs_gpio, 0);
    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

    t->tx_buf = buf;
    t->len = len;
    spi_message_init(&m);
    spi_message_add_tail(t,&m);
    ret = spi_sync(spi,&m);

    kfree(t);
    gpio_set_value(dev->cs_gpio,1);
    return ret;
}

u16 W25QXX_ReadID(struct wq_dev *dev)
{
	u16 Temp = 0;	  
	struct spi_device *spi = (struct spi_device *)dev->private_data;
	// w25q80_write_onereg(dev,0x90);  
	// w25q80_write_onereg(dev,0x00); 	    
	// w25q80_write_onereg(dev,0x00); 	    
	// w25q80_write_onereg(dev,0x00); 	 			   
	// Temp |=w25q80_read_onereg(dev)<<8;  
    // Temp |=w25q80_read_onereg(dev);
    // u8 buf[5] = {0x90,0x00,0x00,0x00,0xff};
    // u8 date=0;
    // spi_write(spi, &buf[0], 1);
	// spi_write(spi, &buf[1], 1);
    // spi_write(spi, &buf[2], 1);
    // spi_write(spi, &buf[3], 1);
    // date = spi_read(spi, &buf[4], 1);
    // printk("W25QXX_ReadID = %d\r\n",date);
    // date = spi_read(spi, &buf[4], 1);
    
    u8 buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
    u8 get[5];
    u8 tx_addr = TX_ADDR;
    u8 write = WRITE_REG2+TX_ADDR;
    int i;
	w25q80_write_regs(dev,&write,1); 	 
    w25q80_write_regs(dev,&buf,5); 			   
	w25q80_read_regs(dev,&tx_addr,1); 
    w25q80_read_regs(dev,&get,5); 
    for(i=0;i<5;i++){
    printk("W25QXX_ReadID = %d\r\n",get[i]);}

	return Temp;
} 


static int oled_open(struct inode *inode, struct file *filp)
{
    // unsigned char ret = 0;
    filp->private_data = &wqdev;
    
    
    return 0;
}

static ssize_t oled_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    struct oeld_dev *dev = (struct wq_dev *)filp->private_data;
    u16 id = 0;
    id = W25QXX_ReadID(dev);

    copy_to_user(buf, &id, sizeof(id));

    return 0;
}

static int oled_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations  oled_ops = 
{
    .owner = THIS_MODULE,
    .open = oled_open,
    .read = oled_read,
    .release = oled_release,
};




static int wqspi_probe(struct spi_device *spi_dev)
{
    int ret;
    printk("probe driver!!! \r\n");

    // 1、构建设备号
    if(wqdev.major)
    {
        wqdev.devid = MKDEV(wqdev.major, 0);
        register_chrdev_region(wqdev.devid,WQ_CNT,WQ_NAME);
    }
    else
    {
        alloc_chrdev_region(&wqdev.devid, 0,WQ_CNT,WQ_NAME);
        wqdev.major = MAJOR(wqdev.devid);
    }

    //2、注册设备
    cdev_init(&wqdev.cdev, &oled_ops);
    cdev_add(&wqdev.cdev, wqdev.devid, WQ_CNT);

    //3、创建类
    wqdev.class = class_create(THIS_MODULE, WQ_NAME);
    if (IS_ERR(wqdev.class))
    {
        return PTR_ERR(wqdev.class);
    }

    //4、创建设备
    wqdev.device = device_create(wqdev.class, NULL, wqdev.devid, NULL, WQ_NAME);
    if (IS_ERR(wqdev.device))
    {
        return PTR_ERR(wqdev.device);
    }

    wqdev.nd = of_find_node_by_path("/soc/aips-bus@02000000/spba-bus@02000000/ecspi@02010000");
    if(wqdev.nd == NULL)
    {
        printk("ecspi3 node not find!\r\n");
        return -EINVAL;
    }
    wqdev.cs_gpio = of_get_named_gpio(wqdev.nd,"cs-gpio",0);
    if(wqdev.cs_gpio < 0)
    {
        printk("can't get cs-gpio");
        return -EINVAL;
    }
    ret = gpio_direction_output(wqdev.cs_gpio, 1);
    if(ret < 0) 
    {
      printk("can't set gpio!\r\n");
    }

    spi_dev->mode = SPI_MODE_0;
    spi_setup(spi_dev);
    wqdev.private_data = spi_dev;

    W25QXX_ReadID(&wqdev);

    return 0;
}

static int wqspi_remove(struct spi_device *spi_dev)
{
    cdev_del(&wqdev.cdev);
    unregister_chrdev_region(wqdev.devid, WQ_CNT);

    device_destroy(wqdev.class,wqdev.devid);
    class_destroy(wqdev.class);

    printk("removing driver\r\n");
    return 0;
}
static const struct of_device_id of_spi_match[] = {
    {.compatible = "alientek,w25q80"},
    {}
};
static const struct spi_device_id spi_idtable[] = {
    {"alientek,w25q80",0},
    {}
};

static const struct spi_driver wq_driver = {
    .probe = wqspi_probe,
    .remove = wqspi_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "wqspi",
        .of_match_table = of_spi_match,
    },
    .id_table = spi_idtable,
};

static int __init wqspi_init(void)
{
    int ret = 0;
    ret = spi_register_driver(&wq_driver);
    return ret;
}

static void __exit wqspi_exit(void)
{
    spi_unregister_driver(&wq_driver);
}

module_init(wqspi_init);
module_exit(wqspi_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");

