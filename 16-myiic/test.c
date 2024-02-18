#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

int fd;
int ret;

int i2c_read_data(unsigned int slave_addr, unsigned char reg_addr)
{
unsigned char data;
//定义一个要发送的数据包 i2c_read_lcd
struct i2c_rdwr_ioctl_data i2c_read_lcd;
//定义初始化 i2c_msg 结构体
struct i2c_msg msg[2] = {
[0] = {
.addr = slave_addr,
.flags = 0,
.buf = &reg_addr,
.len = sizeof(reg_addr)},
[1] = {.addr = slave_addr,
.flags = 1,
.buf = &data,
.len = sizeof(data)},
//设置从机额地址
//设置为写
//设置寄存器的地址
//设置寄存器的地址的长度
//设置从机额地址
//设置为读
//设置寄存器的地址
//设置寄存器的地址
};
//初始化数据包的数据
i2c_read_lcd.msgs = msg;
//初始化数据包的个数
i2c_read_lcd.nmsgs = 2;
//操作读写数据包
ret = ioctl(fd, I2C_RDWR, &i2c_read_lcd);
if (ret < 0)
{
perror("ioctl error ");
return ret;
}
return data;
}

int main(int argc, char *argv[])
{
int TD_STATUS;

fd = open("/dev/i2c-1", O_RDWR);
if (fd < 0)
{
    perror("open error \n");
return fd;
}
while (1)
{
//i2C 读从机地址为 0x38，寄存器地址为 0x02 的数据
//我们从数据手册中得知 TD_STATUS 的地址为 0x02
TD_STATUS = i2c_read_data(0x3c, 0x00);
// 打印 TD_STATUS 的值
printf("TD_STATUS value is %d \n", TD_STATUS);
sleep(1);
}
close(fd);
return 0;
}

