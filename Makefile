obj-m := leds_driver.o
KDIR  := /a_linux_station/kernel/linux-3.5/
#模块最终存放
rootfs_dir :=/a_linux_station/rootfs/home/

all:
	make -C $(KDIR) M=$(PWD) modules  
	arm-linux-gcc app.c -o app 
 
	rm -fr  .tmp_versions *.o *.mod.o *.mod.c  *.bak *.symvers *.markers *.unsigned *.order *~ .*.*.cmd .*.*.*.cmd
	cp *.ko  app ${rootfs_dir}
  
clean:
	@make -C  $(KDIR)   M=$(PWD)  modules  clean
	@rm -f *.ko  app *.o *.mod.o *.mod.c  *.bak *.symvers *.markers *.unsigned *.order *~
