#!/bin/bash  
  
# 检查参数数量  
if [ "$#" -ne 1 ]; then  
    echo "请提供一个参数。"  
    exit 1  
fi  
  
# 检查参数是否为 "option1" 或 "option2"  
if [ "$1" = "1" ]; then  
    echo "编译驱动"  
    make clean
    make
    cp iictest.ko /home/moosa/linuxtest/nfs/rootfs/lib/modules/4.1.15/ -f
elif [ "$1" = "2" ]; then  
    echo "编译文件"  
    rm olapp -v
    arm-linux-gnueabihf-gcc oledapp.c -o olapp
    cp olapp /home/moosa/linuxtest/nfs/rootfs/lib/modules/4.1.15/ -v
else  
    echo "无效的参数"  
    exit 1  
fi