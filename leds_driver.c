#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include "leds_cmd.h"

//设备名定义
#define   DEV_NAME    "wrtdev"
#define   LEDS_SIZE          4    //设备支持的leds数量

static struct cdev *pcdev;        //cdev结构指针
static dev_t    dev_nr;           //存放第一个设备号
static unsigned count = 2;        //设备数量
static struct device *this_device;

//1)定义一个class结构指针
static  struct class *myclass;

//存放物理地址对应的虚拟地址
unsigned int __iomem *   base_addr;  // __iomem 可选择的，告诉你是虚拟地址而已

//寄存器定义
#define REGBASE    (0x110002E0)
#define GPM4CON     (*(volatile unsigned int *)(base_addr))
#define GPM4DAT     (*(volatile unsigned int *)(base_addr + 1))

//成功返回0，失败返回负数
static  int led_open(struct inode *pinode, struct file *pfile) {
    return 0;
}


//成功返回0，失败返回负数
static  int led_release(struct inode *pinode, struct file *pfile) {
    return 0;
}


//成功返回读取的字节数量，失败返回负数
static  ssize_t led_read(struct file *pfile, char __user *buf, size_t size, loff_t * off) {
    int i;
    int ret  = 0;
    char *kbuf;

    loff_t  pos = *off;   //取当前文件指针偏移量
    printk("lld pos:%lld\n", pos);

    //修正用户空间传递下来的非法参数
    if((size == 0) || (pos >= LEDS_SIZE)) {
        return 0;
    }
    if(size + pos > LEDS_SIZE ) {
        size = LEDS_SIZE - pos;  
    }

    //分配空间，还没有数据，全部是 0
    kbuf = kzalloc(size + 1, GFP_KERNEL);
    if(!kbuf) {
        printk("error kmalloc \n");
        ret = -ENOMEM;
        goto error_kmalloc;
    }

    //复制前准备数据: 读取 数据 GPM4DAT 寄存器，分别判断0~3位，0表示亮，1表示灭
    for ( i = 0 ; i < size; i++ ) {
        if(GPM4DAT & 1 << (pos + i)) {
            kbuf[i] = '0'  ;    //返回 ‘0’ 表示灭
        } else {
            kbuf[i] = '1'  ;    //返回 ‘1’ 表示亮
        }
    }

    //复制数据到用户空间
    ret = copy_to_user(buf, kbuf, size);
    if(ret) {
        printk("error copy_to_user \n");
        ret =  -EFAULT;
        goto error_copy_to_user;
    }

    //使用后需要释放空间
    kfree(kbuf);

    //更新文件指针
    *off += size ;

    return  size;

error_copy_to_user:
    kfree(kbuf);
error_kmalloc:
    return ret;
}

//成功返回写入的字节数量，失败返回负数
static  ssize_t led_write(struct file *pfile, const char __user *buf, size_t  size, loff_t * off) {
    int i;
    int ret  = 0;
    char *kbuf;

    loff_t  pos = *off;   //取当前文件指针偏移量
    printk("lld pos:%lld\n", pos);

    //修正用户空间传递下来的非法参数
    if((size == 0)   || (pos >= LEDS_SIZE)) {
        return 0;
    }
    if(size + pos > LEDS_SIZE ) {
        size = LEDS_SIZE - pos;  //2
    }

    //分配空间
    kbuf = kzalloc(size + 1, GFP_KERNEL);
    if(!kbuf) {
        printk("error kmalloc \n");
        ret = -ENOMEM;
        goto error_kmalloc;
    }

    //复制用户空间传递下来的数据:先检测buf指针是否合法，不合法直接返回
    ret = copy_from_user(kbuf, buf, size);
    if(ret) {
        printk("error copy_from_user \n");
        ret =  -EFAULT;
        goto error_copy_from_user;
    }

    //   0,1
    //   2,3
    for ( i = 0 ; i < size ; i++ ) {
        if(buf[i] == '1') {  //亮
            GPM4DAT &= ~(1 << (i + pos)); //清0第i位 低电平亮
        } else if(buf[i] == '0') {
            GPM4DAT |=  (1 << (i + pos));  //设置为1
        } else {
            printk("un change\n");
        }
    }

    //用完释放空间
    kfree(kbuf);

    //修改文件指针
    *off   += size;   //4

    return  size;

error_copy_from_user:
    kfree(kbuf);
error_kmalloc:
    return  ret;
}

#if 0
系统调用接口功能描述
原型：
off_t lseek(int fd, off_t offset, int whence);
功能：根据 whence 和 offset参数设置文件的读写指针位置
参数：
offset：要调整偏移量，具体使用方法和  whence 有关
whence：描述如何使用 offset，可以值如下：
SEEK_SET：把文件读写指针位置设置 offset（以0点为参考点）
SEEK_CUR：把文件读写指针位置设置为当前位置 +offset  （以当前为参考点）
SEEK_END：把文件读写指针位置设置为文件大小 +offset  （以文件末尾为参考点）
返回:
最终文件指针位置
#endif

loff_t  led_llseek(struct file *pfile, loff_t offset, int whence) {
    loff_t  new_pos = pfile->f_pos;   //取出当前值
    switch ( whence ) {
        case SEEK_SET :
            new_pos  = offset;
            break;
        case SEEK_CUR :
            new_pos += offset;
            break;
        case SEEK_END :
            new_pos = LEDS_SIZE + offset;
            break;
        default:
            return -EINVAL;   //参数无效
    }
    //验证最终值是否合法
    if( (new_pos < 0)  ||  (new_pos >= LEDS_SIZE)) {
        return -EINVAL;   //参数无效
    }

    //更新文件指针
    pfile->f_pos  = new_pos;

    //返回的是最终 的文件指针位置
    return new_pos  ;
}



long led_ioctl(struct file *pfile, unsigned int cmd, unsigned long arg) {
  int ret;
	int i;
	char  s[LEDS_SIZE];                    //存放灯状态
	unsigned long pos;

	//不是必须的，只是为了提高CPU运行效率，不做无用判断
	if( _IOC_TYPE(cmd) != LEDS_MAGE){
		printk("error _IOC_TYPE(cmd) \r\n");
		return -EINVAL;
	}
    
	if( _IOC_NR(cmd) > LEDS_CMD_MAX){
		printk("error _IOC_NR(cmd) \r\n");
		return -EINVAL;
	}
    	
  //如果用户空间使用方法第三个参数是传递指针，这里需要还原成原来的地址，取出内容
	//pos = *(unsigned long *)arg;  //直接取是不安全的，
	ret =  copy_from_user(&pos, (const void __user * )arg, _IOC_SIZE(cmd));
	if(ret){
		printk("error copy_from_user \r\n");
		return -EFAULT;
	}

	//如果用户空间使用方法是第三个参数传递的是一个普通数值表示灯灯编号 ，这里直接取来出就可以了
  // pos = arg    
	printk("_IOC_SIZE(cmd):%d\r\n",_IOC_SIZE(cmd));

	switch ( cmd )
	{
	    case LEDS_ALL_OFF :
	        GPM4DAT |=  (0xf << 0);  //设置为1
	        break;
	    case LEDS_ALL_ON :
	        GPM4DAT &=  ~(0xf << 0);  //设置为0
	        break;
	    case LEDS_X_OFF :
	        GPM4DAT |=   (0x1 << pos);  //设置为1
	        break;
	    case LEDS_X_ON :
	        GPM4DAT &=  ~(0x1 << pos);  //设置为0
	        break;
	    case LEDS_X_S :
	        s[0] = !(GPM4DAT & 1<<pos) + '0';   // '0' 或 '1'
          ret = copy_to_user((void __user *) arg, (const void *)&s, _IOC_SIZE(cmd));
          if(ret){
            printk("error copy_from_user \r\n");
            return -EFAULT;
          }
	        break;			
	    case LEDS_ALL_S :
          for ( i = 0 ; i < LEDS_SIZE ; i++ )
          { 
              s[i] = !(GPM4DAT & 1<<i) + '0';	//'0' ,'1'			
          }
          ret = copy_to_user((void __user *) arg, (const void *)&s, _IOC_SIZE(cmd));
          if(ret){
            printk("error copy_from_user \r\n");
            return -EFAULT;
          }	
	        break;				
	    default:
	        return -EINVAL;
	}

	return 0;
}

//核心是实现文件操作方法
static  struct file_operations myfops = {
    .open      = led_open,
    .release   = led_release,
    .read      = led_read,
    .write     = led_write,
    .llseek    = led_llseek,
    .unlocked_ioctl = led_ioctl,
};

//模块安装成功返回 0，负数失败
static int __init mydev_init(void) {
    int ret ;
    int i;

    //1)分配cdev结构空间
    pcdev = cdev_alloc();
    if(!pcdev) {
        printk("error : cdev_alloc\r\n");
        ret = -ENOMEM;
        goto error_cdev_alloc;
    }

    //2)申请设备号
    //register_chrdev_region(dev_t from, unsigned count, const char * name)
    ret = alloc_chrdev_region(&dev_nr, 0, count, DEV_NAME);
    if(ret < 0) {
        printk("error : alloc_chrdev_region\r\n");
        goto error_alloc_chrdev_region;
    }

    //3)初始化cdev结构空间
    cdev_init(pcdev, &myfops);

    //4)注册cdev设备
    ret = cdev_add(pcdev, dev_nr, count);
    if(ret < 0) {
        printk("error : cdev_add\r\n");
        goto error_cdev_add;
    }

    //输出主设备号
    printk("major:%d\n", MAJOR(dev_nr));
    printk(" cdev_add  ok\r\n");

    //创建设备类
    myclass = class_create(THIS_MODULE, "myclass");
    if(IS_ERR(myclass)) {
        ret = PTR_ERR(myclass);
        goto error_class_create;
    }

    //创建/dev/目录下设备文件
    for ( i = 0 ; i < count ; i++ ) {
        this_device = device_create(myclass, NULL, dev_nr + i, pcdev, "%s%d", "myleds", i); // myleds0 myleds1
        if(IS_ERR(this_device)) {
            ret = PTR_ERR(this_device);
            goto error_device_create;
        }
    }

    //映射寄存器地址空间为虚拟地址空间
    base_addr = ioremap(REGBASE, 8);
    if(!base_addr) {
        printk("error ioremap\n");
        ret =  -ENOMEM;
        goto error_ioremap;
    }

    //初始化
    GPM4CON &= ~((0xf << 3 * 4) | (0xf << 2 * 4) | (0xf << 1 * 4) | (0xf << 0 * 4));
    GPM4CON |=  ((1  << 3 * 4) | (1  << 2 * 4) | (1  << 1 * 4) | (1  << 0 * 4));

    //初始默认灭
    GPM4DAT |=  0XF;

    return 0;

error_ioremap:
    //删除/dev/目录下的设备文件
    for ( i = 0 ; i < count ; i++ ) {
        device_destroy(myclass, dev_nr + i);
    }

error_device_create:
    class_destroy(myclass);
error_class_create:
    cdev_del(pcdev);
error_cdev_add:
    unregister_chrdev_region(dev_nr, count);
error_alloc_chrdev_region:
    kfree(pcdev);     //释放分配的空间
error_cdev_alloc:
    return ret;
}

static void  __exit mydev_exit(void) {
    int i;

    iounmap(base_addr);   //取消映射

    //删除/dev/目录下的设备文件
    for ( i = 0 ; i < count ; i++ ) {
        device_destroy(myclass, dev_nr + i);
    }

    //销毁设备类
    class_destroy(myclass);

    //1) 注销cdev设备
    cdev_del(pcdev);

    //2) 释放设备号
    unregister_chrdev_region(dev_nr, count);

    //3) 释放cdev空间
    kfree(pcdev);

    printk(" unregister_chrdev  ok\r\n");
}

module_init(mydev_init);
module_exit(mydev_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("xyd");
MODULE_DESCRIPTION("misc device");
