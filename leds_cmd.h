#ifndef __LEDS_CMD_H__
#define __LEDS_CMD_H__
#include <linux/ioctl.h>
#if 0
#define LEDS_ALL_OFF    0          //关全部灯
#define LEDS_ALL_ON     1          //开全部灯
#define LEDS_X_OFF      2          //关指定灯
#define LEDS_X_ON       3          //开指定灯
#endif

#define LEDS_MAGE       'L'                              //魔数
#define LEDS_CMD_MAX     5                               //最大命令序号
#define LEDS_ALL_OFF    _IO(LEDS_MAGE, 0)                //关全部灯
#define LEDS_ALL_ON     _IO(LEDS_MAGE, 1)                //开全部灯
#define LEDS_X_OFF      _IOW(LEDS_MAGE, 2,long)          //关指定灯
#define LEDS_X_ON       _IOW(LEDS_MAGE, 3,long)          //开指定灯
#define LEDS_X_S        _IOR(LEDS_MAGE, 4,char)          //读指定灯状态
#define LEDS_ALL_S      _IOR(LEDS_MAGE, 5,long)          //读全部灯状态

#endif
