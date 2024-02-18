#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "linux/ioctl.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <signal.h> 
#include <fcntl.h>

/*
 * @description		: main主程序
 * @param - argc 	: argv数组元素个数
 * @param - argv 	: 具体参数
 * @return 			: 0 成功;其他 失败
 */
int main(int argc, char *argv[])
{
	int fd, ret,cnt = 0;
	char *filename;
	unsigned short databuf[3];
	
	
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
		ret = read(fd , databuf, sizeof(databuf));
		if(ret == 0)
		{

			// printf("read data = %d\r\n", databuf[0]);
			printf("runing\r\n");

		}
		usleep(200000);
		cnt ++;
		if(cnt  = 5)
		{
			break;
		}
		
	}
	
	ret = close(fd); /* 关闭文件 */
	if(ret < 0){
		printf("file %s close failed!\r\n", argv[1]);
		return -1;
	}
	return 0;
}


