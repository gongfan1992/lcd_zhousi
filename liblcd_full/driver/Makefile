
##############################################################################

ifneq ($(KERNELRELEASE),)

obj-m		:= generic_pcie.o

else

ifeq ($(platform),)
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
else
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
endif

PWD := $(shell pwd)

.PHONY: all modules clean install

all: modules
	@cp generic_pcie.ko ../build/

modules:
	@echo "Start to compile the driver module."
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f Module.* modules.order

install:
	
endif



