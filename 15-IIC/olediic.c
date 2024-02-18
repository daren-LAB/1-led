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

#include "show.h"


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

static int oled_read_regs(struct oled_dev *dev, u8 reg, void *val, int len)
{
    int ret = 0 ;
    struct i2c_msg msg[2];
    struct i2c_client *client = (struct i2c_adapter *)dev->private_data;

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;
    
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter,msg,2);
    if(ret == 2)
    {
        ret = 0;
    }else{
        printk("i2c read failed: %d    reg = %06d   len = %d\n",ret,reg,len);
        ret = -EREMOTEIO;
    }

    return ret;
}

static int oled_write_regs(struct oled_dev *dev, u8 reg, u8 *buf, u8 len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client *client = (struct i2c_adapter *)dev->private_data;    

    b[0] = reg;
    memcpy(&b[1], buf ,len);

    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 1;

    return i2c_transfer(client->adapter,&msg,1);
}

static unsigned char oled_read_reg(struct oled_dev *dev, u8 reg)
{
    u8 date = 0;
    oled_read_regs(dev,reg,&date, 1);
    return date;
    // struct i2c_client *client = (struct i2c_client *)dev->private_data;
    // return i2c_smbus_read_byte_data(client, reg);
}
static void oled_write_reg(struct oled_dev *dev, u8 reg, u8 data)
{
    u8 buf = 0;
    buf = data;
    oled_write_regs(dev, reg, &buf, 1);
}



void WriteCmd(unsigned char I2C_Command)//写命令
{	
	oled_write_reg( &oleddev, 0x00,  I2C_Command);
}

void WriteDat(unsigned char I2C_Data)//写数据
{
	oled_write_reg( &oleddev, 0x40,  I2C_Data);
}
void OLED_REGInit(void)
{
	mdelay(1000);
	WriteCmd(0xAE); //display off
	WriteCmd(0x00);	//Set Memory Addressing Mode	
	WriteCmd(0x10);	//00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
	WriteCmd(0x40);	//Set Page Start Address for Page Addressing Mode,0-7
	WriteCmd(0xb0);	//Set COM Output Scan Direction
	WriteCmd(0x81); //---set low column address
	WriteCmd(0xff); //---set high column address
	WriteCmd(0xa1); //--set start line address
	WriteCmd(0xa6); //--set contrast control register
	WriteCmd(0xa8); //亮度调节 0x00~0xff
	WriteCmd(0xc8); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
	WriteCmd(0xd3); //-set display offset
	WriteCmd(0x00); //-not offset

	WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
	WriteCmd(0x80); //--set divide ratio

    WriteCmd(0xd8); //--set display clock divide ratio/oscillator frequency
	WriteCmd(0x05); //--set divide ratio

	WriteCmd(0xd9); //--set pre-charge period
	WriteCmd(0xf1); //

	WriteCmd(0xda); //--set com pins hardware configuration
	WriteCmd(0x12);

	WriteCmd(0xdb); //--set vcomh
	WriteCmd(0x30); //0x20,0.77xVcc

	WriteCmd(0x8d); //--set DC-DC enable
	WriteCmd(0x14); //
	WriteCmd(0xaf); //--turn on oled panel
	 printk("oled reginit................\r\n");
}
void OLED_SetPos(unsigned char x, unsigned char y) //设置起始点坐标
{ 
	WriteCmd(0xb0+y);
	WriteCmd(((x&0xf0)>>4)|0x10);
	WriteCmd((x&0x0f)|0x01);
}
// Parmeters     : x,y -- 起始点坐标(x:0~127, y:0~7); ch[] -- 要显示的字符串;
void OLED_ShowStr(unsigned char x, unsigned char y, unsigned char ch[])
{
	unsigned char c = 0,i = 0,j = 0;
		
    while(ch[j] != '\0')
    {
        c = ch[j] - 32;
        if(x > 126)
        {
            x = 0;
            y++;
        }
        OLED_SetPos(x,y);
        for(i=0;i<6;i++)
            WriteDat(F6x8[c][i]);
        x += 6;
        j++;
    }
		
}



static int oled_open(struct inode *inode, struct file *filp)
{
    unsigned char ret = 0;
    filp->private_data = &oleddev;


    printk("open finished\r\n");

    return 0;
}

static ssize_t oled_read(struct file *filp, char __user *buf,
                            size_t cnt, loff_t *off)
{
    struct oeld_dev *dev = (struct oeld_dev *)filp->private_data;
    long err = 0;
    unsigned char ret = 0;

    

    OLED_REGInit();
   

    OLED_ShowStr(60, 1, "lsllsdlsadas");
   
    
    oled_write_reg( dev, 0x40, 0xff);
    ret = oled_read_reg(dev, 0x40 );
    printk("Oled kernel read =  %d\r\n",ret);
    // err = copy_to_user(buf, , sizeof());
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

    return 0;
}

static int oled_remove(struct i2c_client *client)
{
    printk("remove driver!!! \r\n");

    cdev_del(&oleddev.cdev);
    unregister_chrdev_region(oleddev.devid, OLED_CNT);
    device_destroy(oleddev.class, oleddev.devid);
    class_destroy(oleddev.class);

    return 0;
}

static const struct of_device_id oled_of_match[] = 
{
    { .compatible = "alientek,oled"},
    { }
};
static const struct i2c_device_id oled_id[] = 
{
    { "alientek,oled",0},
    { }
};

static struct i2c_driver oled_driver = 
{
    .probe = oled_probe,
    .remove = oled_remove,
    .driver = {
		.owner = THIS_MODULE,
		.name = "oled",
		.of_match_table = oled_of_match,
	},
    .id_table = oled_id,

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
