#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"

#define LEDOFF 	0
#define LEDON 	1
#define close_cmd    (_IO(0xEF, 0x1))
#define open_cmd     (_IO(0xEF, 0x2))
#define setpriod_cmd (_IO(0xEF, 0x3))

/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret;
	char *filename;
	unsigned long cmd,arg;
	unsigned char str[100];
	
	if(argc != 2){
		printf("Error Usage!\r\n");
		return -1;
	}

	filename = argv[1];

	/* 打开led驱动 */
	fd = open(filename, O_RDWR);
	if(fd < 0){
		printf("file %s open failed!\r\n", argv[1]);
		return -1;
	}

	while(1)/* 要执行的操作：打开或关闭 */
	{
		printf("Input cmd: ");
		ret = scanf("%d", &cmd);
		if(ret !=0 )
		{
			gets(str);
		}

		if(cmd == 1)
		{
			cmd = close_cmd;
		}else if(cmd == 2)
		{
			cmd = open_cmd;
		}else if(cmd == 3)
		{
			cmd = setpriod_cmd;
			printf("Input timer period :");
			ret = scanf("%d", &arg);
			if(ret != 1)
			{
				gets(str);
			}
		}
		ioctl(fd, cmd, arg);
	}
	
	ret = close(fd); /* 关闭文件 */
	if(ret < 0){
		printf("file %s close failed!\r\n", argv[1]);
		return -1;
	}
	return 0;
}


