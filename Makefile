DRIVER = input
KERNELDIR=/home/gwang/raspi_tool/kernel-3.10
CROSS_COMPILE=arm-linux-gnueabi-
#CFLAGS="-march=armv6"
#KERNELDIR=/lib/modules/3.6.11+/kernel
ifneq ($(KERNELRELEASE),)
    obj-m := $(DRIVER).o
else
    PWD := $(shell pwd)

default:
ifeq ($(strip $(KERNELDIR)),)
	$(error "KERNELDIR is undefined!")
else
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
endif


clean:
	rm -rf *~ *.ko *.o *.mod.c modules.order Module.symvers .$(DRIVER)* .tmp_versions

endif

