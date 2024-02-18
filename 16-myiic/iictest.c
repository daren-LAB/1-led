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
#include <linux/i2c.h>

#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of_gpio.h>


#define OLED_CMD  0	//写命令
#define OLED_DATA 1	

#define OLED_CNT 1
#define OLED_NAME "oled"


struct oled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int major;
    void *private_data;

};
struct oled_dev oleddev;


/*------------------读写函数------------------*/
static int my_read_regs(struct oled_dev *dev, 
                        u8 reg,
                        void *val, 
                        int len)
{
    int ret = 0;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_client *)dev->private_data;

    msg[0].addr = client->addr; /* ap3216c 地址 */
    msg[0].flags = 0; /* 标记为发送数据 */
    msg[0].buf = &reg; /* 读取的首地址 */
    msg[0].len = 1; /* reg 长度 */
    /* msg[1]读取数据 */ 
    msg[1].addr = client->addr; /* ap3216c 地址 */
    msg[1].flags = I2C_M_RD; /* 标记为读取数据 */
    msg[1].buf = val; /* 读取数据缓冲区 */
    msg[1].len = len; /* 要读取的数据长度 */
    ret = i2c_transfer(client->adapter, msg, 2); 
    if(ret == 2) 
    { 
        ret = 0;
    } else
     {
     printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
     ret = -EREMOTEIO;
    }
     return ret;
}

static s32 my_write_regs(struct oled_dev *dev, u8 reg, u8 *buf,u8 len)
{
    int ret = 0;
	u8 b[2];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* 寄存器首地址 */

   
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
    
	msg.addr = client->addr;	/* ap3216c地址 */
    printk("addr: %d\n", client->addr);

	msg.flags = 0;				/* 标记为写数据 */
	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	ret = i2c_transfer(client->adapter, &msg, 1);
    if(ret < 0) 
    { 
     printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
    }
    return ret;
}


// void OLED_Clear(struct oled_dev *dev)  
// {  
// 	u8 i,n;		    
// 	for(i=0;i<8;i++)  
// 	{  
// 		OLED_WR_Byte (dev,0xb0+i,OLED_CMD);    //设置页地址（0~7）
// 		OLED_WR_Byte (dev,0x00,OLED_CMD);      //设置显示位置—列低地址
// 		OLED_WR_Byte (dev,0x10,OLED_CMD);      //设置显示位置—列高地址   
// 		for(n=0;n<128;n++)OLED_WR_Byte(dev,0,OLED_DATA); 
// 	} //更新显示
// }
// void OLED_On(struct oled_dev *dev)  
// {  
// 	u8 i,n;		    
// 	for(i=0;i<8;i++)  
// 	{  
// 		OLED_WR_Byte (dev,0xb0+i,OLED_CMD);    //设置页地址（0~7）
// 		OLED_WR_Byte (dev,0x00,OLED_CMD);      //设置显示位置—列低地址
// 		OLED_WR_Byte (dev,0x10,OLED_CMD);      //设置显示位置—列高地址   
// 		for(n=0;n<128;n++)OLED_WR_Byte(dev,0xff,OLED_DATA); 
// 	} //更新显示
// }

static int oled_open(struct inode *inode, struct file *filp)
{
    unsigned char ret = 0;
    filp->private_data = &oleddev;
    u8 date = 0xff;
    u8 get = 0;
   
    get = i2c_master_send(oleddev.private_data, &date, 1);

    // my_write_regs(&oleddev,0x40,&date,1);
    // my_read_regs(&oleddev, 0x40 ,&get , 1);
    printk("get = %d\n",get);

#if 0
    OLED_WR_Byte(&oleddev,0xAE,OLED_CMD);//--display off
	OLED_WR_Byte(&oleddev,0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(&oleddev,0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(&oleddev,0x40,OLED_CMD);//--set start line address  
	OLED_WR_Byte(&oleddev,0xB0,OLED_CMD);//--set page address
	OLED_WR_Byte(&oleddev,0x81,OLED_CMD); // contract control
	OLED_WR_Byte(&oleddev,0xFF,OLED_CMD);//--128   
	OLED_WR_Byte(&oleddev,0xA1,OLED_CMD);//set segment remap 
	OLED_WR_Byte(&oleddev,0xA6,OLED_CMD);//--normal / reverse
	OLED_WR_Byte(&oleddev,0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(&oleddev,0x3F,OLED_CMD);//--1/32 duty
	OLED_WR_Byte(&oleddev,0xC8,OLED_CMD);//Com scan direction
	OLED_WR_Byte(&oleddev,0xD3,OLED_CMD);//-set display offset
	OLED_WR_Byte(&oleddev,0x00,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xD5,OLED_CMD);//set osc division
	OLED_WR_Byte(&oleddev,0x80,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xD8,OLED_CMD);//set area color mode off
	OLED_WR_Byte(&oleddev,0x05,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xD9,OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte(&oleddev,0xF1,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xDA,OLED_CMD);//set com pin configuartion
	OLED_WR_Byte(&oleddev,0x12,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xDB,OLED_CMD);//set Vcomh
	OLED_WR_Byte(&oleddev,0x30,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0x8D,OLED_CMD);//set charge pump enable
	OLED_WR_Byte(&oleddev,0x14,OLED_CMD);//
	
	OLED_WR_Byte(&oleddev,0xAF,OLED_CMD);//--turn on oled panel
#endif 
    printk("open finished\r\n");

    
    return 0;
}

static ssize_t oled_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    struct oeld_dev *dev = (struct oeld_dev *)filp->private_data;
    int value = 0x11;


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

static int oled_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
    u8 date = 0xff;
    printk("probe driver!!! \r\n");

    // 1、构建设备号
    if(oleddev.major)
    {
        oleddev.devid = MKDEV(oleddev.major, 0);
        register_chrdev_region(oleddev.devid,OLED_CNT,OLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&oleddev.devid, 0,OLED_CNT,OLED_NAME);
        oleddev.major = MAJOR(oleddev.devid);
    }

    //2、注册设备
    cdev_init(&oleddev.cdev, &oled_ops);
    cdev_add(&oleddev.cdev, oleddev.devid, OLED_CNT);

    //3、创建类
    oleddev.class = class_create(THIS_MODULE, OLED_NAME);
    if (IS_ERR(oleddev.class))
    {
        return PTR_ERR(oleddev.class);
    }

    //4、创建设备
    oleddev.device = device_create(oleddev.class, NULL, oleddev.devid, NULL, OLED_NAME);
    if (IS_ERR(oleddev.device))
    {
        return PTR_ERR(oleddev.device);
    }
    oleddev.private_data = client;
    my_write_regs(&oleddev,0x40,&date,1);

    return 0;
}

static int oled_remove(struct i2c_client *client)
{
    cdev_del(&oleddev.cdev);
    unregister_chrdev_region(oleddev.devid, OLED_CNT);

    device_destroy(oleddev.class,oleddev.devid);
    class_destroy(oleddev.class);
    return 0;
}
static const struct of_device_id of_oled_match[] = {
    {.compatible = "alientek,oled"},
    {}
};
static const struct i2c_device_id i2c_idtable[] = {
    {"alientek,oled",0},
    {}
};

static const struct i2c_driver oled_driver = {
    .probe = oled_probe,
    .remove = oled_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "oled",
        .of_match_table = of_oled_match,
    },
    .id_table = i2c_idtable,

};

static int __init oeldi2c_init(void)
{
    int ret = 0;
    ret = i2c_add_driver(&oled_driver);
    return ret;
}

static void __exit oeldi2c_exit(void)
{
    i2c_del_driver(&oled_driver);
}

module_init(oeldi2c_init);
module_exit(oeldi2c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("moosa");
