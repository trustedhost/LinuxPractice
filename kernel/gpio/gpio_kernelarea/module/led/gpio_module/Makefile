KDIR = /lib/modules/`uname -r`/build

obj-m += gpio_module_new.o

default:
	$(MAKE) -C $(KDIR) M=$$PWD modules
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
