#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "leds_cmd.h"

//./app /dev/myleds0
int main(int argc, char * argv[]) {
    int  fd ;
    int i, ret;
    char recv_buf[100] = {0}, send_buf[100] = {"1010"};
    //char *p=(char*)10;  //非法指针测试使用
    
    if(argc != 2) {
        printf("Usage:%s /dev/xxx\r\n", argv[0]);
        return -1;
    }

    //应用每open一次会在内核中创建一个 struct file 结构，
    //这个结构用户空间是看不到的
    //后面可以通过fd来找到  struct file ，所以后面的read,write函数第一个参数都是fd
    fd = open(argv[1], O_RDWR);
    if(fd < 0) {
        perror("open");
        return -1;
    }

    while(1) {
        for ( i = 0 ; i < 4 ; i++ ) {
            unsigned long   argp = i;                    //目标第一个led
            ioctl(fd, LEDS_X_ON, ( unsigned long)&argp);  //关指定灯，这种用法第三个参数是指针
            sleep(1);
        }

        for ( i = 0 ; i < 4 ; i++ ) {
            unsigned long   argp = i;                     //目标第一个led
            ioctl(fd, LEDS_X_OFF, ( unsigned long)&argp);  //关指定灯，这种用法第三个参数是指针
            sleep(1);
        }

        ioctl(fd, LEDS_ALL_ON);  //开全部灯，这种用法第三个参数是指针
        sleep(1);
		
        ioctl(fd, LEDS_ALL_OFF);  //关全部灯，这种用法第三个参数是指针
        sleep(1);
		
    }

    close(fd);
}

