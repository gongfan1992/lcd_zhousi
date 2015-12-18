/*************************************************************************
	> File Name: generic_pcie.c
	> Author: 
	> Mail: 
	> Created Time: Wed 19 Nov 2014 04:16:55 PM CST
 ************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include "../include/common.h"
#include "generic_pcie.h"
#define GENERIC_PCIE_NAME "generic_pcie_device"
#define GENERIC_PCIE_NUMS 1
struct generic_dev gdev_body;
struct generic_dev * gdev = &gdev_body;
int global_trace = TR_F_ERROR;

static unsigned const sus_pins[] = {
	29, 33, 30, 31, 32, 34, 36, 35, 38, 37,
	18, 7, 11, 20, 17, 1, 8, 10, 19, 12,
	0, 2, 23, 39, 28, 27, 22, 21, 24, 25,
	26, 51, 56, 54, 49, 55, 48, 57, 50, 58,
	52, 53, 59, 40,
};
static struct pci_device_id pcie_generic_table[] __initdata = 
{
    {
        .vendor = GENERIC_PCIE_VENDORID,
        .device = GENERIC_PCIE_DEVICEID,
        .subvendor = PCI_ANY_ID,
        .subdevice = PCI_ANY_ID,
    },
    { /* all zeros */ }
};
MODULE_DEVICE_TABLE(pci, pcie_generic_table);
static int generic_pcie_read_ulong(struct generic_dev * gdev, unsigned long arg)
{
    int result = -1;
    iomsg_t msg;
    void __iomem * reg;
    u32 old_val = 0;
    /** Copy the message from user stack. */
    result = access_ok(VERIFY_WRITE, (void*)arg, sizeof(iomsg_t));
    if (!result)
    {
                TRACE(TR_F_ERROR, "Access to the message from user stack is invalid.\n");
                return -1;
    }
    result = copy_from_user(&msg, (void*)arg, sizeof(iomsg_t));
    if (result != 0)
    {
                TRACE(TR_F_ERROR, "Try to copy the message from user stack failed.\n");
                return -1;
    }
    /** Read the register here. */
	reg = gdev->cfg.map[0] + 0x2000 + sus_pins[msg.index]*16 + 8;
    old_val = readl(reg);
	old_val &= ~0x4;
	old_val |= 0x2;
	writel(old_val,reg);

    msg.data = readl(reg) & 0x1;
    result = copy_to_user((void*)arg, &msg, sizeof(iomsg_t));
    if (result != 0)
    {
        TRACE(TR_F_ERROR, "Return value of the register failed.\n");
        return -1;
    }
    return 0;
}

static int generic_pcie_write_ulong(struct generic_dev * gdev, unsigned long arg)
{
	int result = -1;
    void __iomem * reg;
    u32 old_val = 0;
	iomsg_t msg;
	/** Copy the message from user stack. */
	result = access_ok(VERIFY_WRITE, (void*)arg, sizeof(iomsg_t));
	if (!result)
	{
		TRACE(TR_F_ERROR, "Access to the message from user stack is invalid.\n");
		return -1;
	}
	result = copy_from_user(&msg, (void*)arg, sizeof(iomsg_t));
	if (result != 0)
	{
		TRACE(TR_F_ERROR, "Try to copy the message from user stack failed.\n");
		return -1;
	}
	/** [ISSUE]: We should check the alignment here. */
	reg = gdev->cfg.map[0] + 0x2000 + sus_pins[msg.index]*16 + 8;
    old_val = readl(reg);
	old_val |= 0x4;
	old_val &= ~0x2;
	writel(old_val,reg);
    if(msg.data)
    {
	    writel(old_val | 0x1,reg);
    }
    else
    {
	    writel(old_val & ~0x1,reg);
    }

	result = copy_to_user((void*)arg, &msg, sizeof(iomsg_t));
	if (result != 0)
	{
		TRACE(TR_F_ERROR, "Return value of the register failed.\n");
		return -1;
	}
	return 0;
}
static int generic_pcie_open(struct inode * inode, struct file * filp)
{
    struct generic_dev * gdev = &gdev_body;
    filp->private_data = gdev;

    down(&gdev->sem);
    gdev->ref++;
    up(&gdev->sem);
    return 0;
}
static int generic_pcie_close(struct inode * inode, struct file * filp)
{
    struct generic_dev * gdev = filp->private_data;
    down(&gdev->sem);
    gdev->ref--;
    up(&gdev->sem);
    return 0;
}


/*
*  此驱动使用内存的流程是这样的
*  1. ioctl 控制驱动分配内存，这个时候的内存是内核的内存。
*     至此，我们可以认为这段内存是属于设备的内存。
*  2. 我们对此设备调用mmap。
*  3. 把mmap的内存用链表给挂载起来。
*  4. 依照使用链表来使用内存。
*
*  假如属于设备的内存不用mmap挂载起来，那么应该如何访问呢？
*  其实我猜想应该是调用read，或者write，或者ioctl。
*  但是此驱动都没有调用这个接口。
*  所以用mmap。
*
*  在此，我的初步想法是这样的。
*  分配N个buffer，当然是2,3份。
*  哎呀，到底几份啊，但是系数矩阵是初始化的时候才需要的啦，
*  好烦啊，看看一个链表使用的过程中是否会用到锁啦。
* */
static long generic_pcie_ioctl(struct file * fp, unsigned int cmd, unsigned long arg)
{
    int stat = -1;
    struct generic_dev * gdev = &gdev_body;
    if(_IOC_TYPE(cmd) != GENERIC_PCIE_TYPE)
    {
        TRACE(TR_F_ERROR, "The ioctl type is not supported.\n");
        return -1;
    }
    switch(cmd)
    {
        case IOCTL_READ_ULONG:
        stat = generic_pcie_read_ulong(gdev,arg);
        break;
        case IOCTL_WRITE_ULONG:
        stat = generic_pcie_write_ulong(gdev,arg);
        break;
        default:
        /** Just notify the unsupported command. */
        TRACE(TR_F_ERROR, "Unsupported command.\n");
        return -1;
    }
    return stat;
}
static const struct file_operations generic_pcie_ops = 
{
    .owner            = THIS_MODULE,
    .open             = generic_pcie_open,
    .release          = generic_pcie_close,
    .unlocked_ioctl   = generic_pcie_ioctl,
};
static int __init generic_init(void)
{
    int result = -1;
    u32 value = 0;
    u32 base = 0;
    struct pci_dev * dev;
    dev = pci_get_device(GENERIC_PCIE_VENDORID,
                        GENERIC_PCIE_DEVICEID,
                        NULL);
    if (dev == NULL)
    {
        TRACE(TR_F_ERROR, "Register the pcie driver for atom failed.\n");
        return -1;
    }
        
    result = pci_read_config_dword(dev,0x4c,&value);
    if(result)
    {
        TRACE(TR_F_ERROR, "Read config dword from atom failed.\n");
        return -1;
    }
    TRACE(TR_F_ERROR, "Read config dword from atom : 0x%x.\n",value);
	base = value & ~0x3fff;
	
    gdev->cfg.typ[0] = IORESOURCE_MEM;
    gdev->cfg.map[0] = (u8*)ioremap_nocache(base,16 * 1024); 
	/** Setup the character device. */
	result = alloc_chrdev_region(&gdev->first, 0, 
								 GENERIC_PCIE_NUMS, 
								 GENERIC_PCIE_NAME);
	if (result != 0)
	{
		TRACE(TR_F_ERROR, "Allocate the region for character device failed.\n");
		goto alloc_cregion_err;
	}
	cdev_init(&gdev->cdev, &generic_pcie_ops);
	gdev->cdev.owner = THIS_MODULE;
	result = cdev_add(&gdev->cdev, gdev->first, GENERIC_PCIE_NUMS);
	if (result != 0)	
	{
		TRACE(TR_F_ERROR, "Add the character driver failed.\n");
		goto add_cdev_err;
	}
	gdev->cfg.slot = PCI_SLOT(dev->devfn);
	gdev->cfg.func = PCI_FUNC(dev->devfn);
	gdev->pdev = dev;
	pci_set_drvdata(dev, gdev);
   	sema_init(&gdev->sem,1);


   	return 0;
add_cdev_err:
	unregister_chrdev_region(gdev->first, GENERIC_PCIE_NUMS);
alloc_cregion_err:
	/** Unmap the I/O memory. */
	iounmap((void*)gdev->cfg.map[0]);
	pci_release_regions(dev);
	return -1;
}
static void __exit generic_exit(void)
{
	/** Unmap the I/O memory. */
#if 1
    struct generic_dev * gdev = &gdev_body;
    unregister_chrdev_region(gdev->first, 1);
    cdev_del(&gdev->cdev);
	iounmap((void*)gdev->cfg.map[0]);
    TRACE(TR_F_REMOVE, "Unmap the I/O memory of the PCIE success.\n");
    pci_release_regions(gdev->pdev);
#endif
}
module_init(generic_init);
module_exit(generic_exit);  
MODULE_LICENSE("GPL");
