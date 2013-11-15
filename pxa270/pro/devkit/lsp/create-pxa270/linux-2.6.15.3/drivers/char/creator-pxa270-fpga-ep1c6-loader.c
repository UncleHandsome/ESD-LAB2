// --------------------------------------------------------------------
//
//   Title     :  creator-pxa270-fpga-ep1c6-loader.c
//             :
//   Library   :
//             :
//   Developers:  MICROTIME MDS group
//             :
//   Purpose   :  Driver for FPGA-EP1C6
//             :
//   Limitation:
//             :
//   Note      :
//             :
// --------------------------------------------------------------------
//   modification history :
// --------------------------------------------------------------------
//   Version| mod. date: |
//   Vx.x   | mm/dd/yyyy |
//   V1.01  | 06/19/2006 | First release
// --------------------------------------------------------------------
//
// Note:
//
//       MICROTIME COMPUTER INC.
//
//
/*************************************************************************
Include files
*************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/cdev.h>
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>

#include <linux/mm.h>
#include <linux/sched.h>	
#include <linux/timer.h>	
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <asm/irq.h>
#include <asm/param.h>	
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include "asm/arch/lib/creator_pxa270_core.h"
#include "asm/arch/lib/creator_pxa270_fpga_ep1c6_loader.h"


/*************************************************************************
Constant define
*************************************************************************/

/* ****** Debug Information ******************************************** */
#define DEBUG
#ifdef DEBUG 
#define MSG(string, args...) printk(KERN_ALERT string, ##args)
#else   
#define MSG(string, args...)
#endif



/* ****** Linux Information ******************************************** */
#if LINUX_VERSION_CODE < 0x020100
#define GET_USER(a,b)   a = get_user(b)
#else
#include <asm/uaccess.h>
#define GET_USER(a,b)   get_user(a,b)
#endif



/* ****** Module Information ******************************************* */
#define	MAJOR_NUM			        FPGA_MAJOR_NUM
#define MAX_MINORS                  1

#define MODULE_VERSION_INFO	        "1.01"
#define MODULE_NAME			        "CREATOR_FGPA"
#define COPYRIGHT			         "Copyright (C) 2006 Microtime Computer Inc."
#define MODULE_AUTHOR_STRING		"Microtime Computer Inc."
#define MODULE_DESCRIPTION_STRING	"Creator FPGA module"


struct  fpga_sbihdr{
	unsigned int fpos;
	unsigned int temp;
	unsigned long checksum;
};



/*************************************************************************
Function prototypes
*************************************************************************/


/*************************************************************************
Variable define
*************************************************************************/
static DECLARE_MUTEX(fpga_sem);
/* ************************************************************************** */



static int 
place_fpga_into_configure_mode (void)
{
        unsigned long flags;         
       
        spin_lock_irqsave(&creator_io.creator_lock, flags);                 
        creator_io.cpld_ctrl &= 0xcf;		//PGM=0, clk=0              
        CPLD_CTRL = creator_io.cpld_ctrl;                
        creator_io.cpld_ctrl |= 0x10;		//PGM=1                    
        CPLD_CTRL = creator_io.cpld_ctrl;            
        spin_unlock_irqrestore(&creator_io.creator_lock, flags);              
        
        return (0);
}    
/* ************************************************************************** */



static int 
write_fpga_data_byte (struct fpga_sbihdr* sbihdr, unsigned char config_data)
{
        UI din;
        UI mask;
        unsigned long flags;                
          
        din = config_data ;
        sbihdr->checksum += din;      

        spin_lock_irqsave(&creator_io.creator_lock, flags);        
        for (mask=1; mask<0x100; mask<<=1) {              
            if ((mask & din) == 0) {
                creator_io.io_reg0 &= 0xbfff;	//din = 0
            }
            else {
                creator_io.io_reg0 |= 0x4000;	//din = 1
            }
            IO_REG0 = creator_io.io_reg0;                       
            creator_io.cpld_ctrl |= 0x20;		//CLK = 1            
            CPLD_CTRL = creator_io.cpld_ctrl;
            creator_io.cpld_ctrl &= 0xdf;		//CLK = 0
            CPLD_CTRL = creator_io.cpld_ctrl;                
        }                    
        spin_unlock_irqrestore(&creator_io.creator_lock, flags);            
        
        return (0);
}
/* ************************************************************************** */



static int 
wait_for_device_to_configure (void)
{
        unsigned long flags;
            
        spin_lock_irqsave(&creator_io.creator_lock, flags);            
        creator_io.cpld_ctrl |= 0x30;		//PGM=1, clk=1
        CPLD_CTRL = creator_io.cpld_ctrl;
		creator_io.io_reg0 |= 0x4000;
		IO_REG0 = creator_io.io_reg0;		
        spin_unlock_irqrestore(&creator_io.creator_lock, flags);	
        		
		if ((IO_REG1 & 0x8000) != 0) 
		    return(0); //FPGA load OK

        // Reset FPGA
        spin_lock_irqsave(&creator_io.creator_lock, flags);          
        creator_io.cpld_ctrl &= 0xcf;		//PGM=0, clk=0
        CPLD_CTRL = creator_io.cpld_ctrl;
        creator_io.cpld_ctrl |= 0x10;		//PGM=1
        CPLD_CTRL = creator_io.cpld_ctrl;    
        spin_unlock_irqrestore(&creator_io.creator_lock, flags);  
                       

        printk(KERN_ALERT "fpga: Invalid FPGA config data\n");        
        return (-EILSEQ);      
}
/* ************************************************************************** */




static int 
fpga_download_code (creator_fpga_download_t *info, struct fpga_sbihdr* sbihdr)
{
        unsigned long   i, len;
        char            *pData ;
        int             result ;
          
        place_fpga_into_configure_mode();  
        len = info->dwSize;
        pData = info->pBuffer;
        for (i=0; i < len; i++){
            write_fpga_data_byte(sbihdr, *pData++);
            sbihdr->fpos++;
        }    
        
        result = wait_for_device_to_configure();             
        
        return (result);
}
/* ************************************************************************** */


     
static int 
fpga_open (struct inode* inode, struct file *filep)
{
        struct fpga_sbihdr* sbihdr;

        /* Check the device minor number */

        /* Create our private data and link it to the file structure */
        sbihdr = kmalloc(sizeof(struct fpga_sbihdr), GFP_KERNEL);

        if (!sbihdr)
            return -ENOMEM;

        filep->private_data = sbihdr;

        memset(sbihdr, 0, sizeof(struct fpga_sbihdr));   
        
        return (0);
}
/* ************************************************************************** */



static int 
fpga_release (struct inode* inode, struct file *filep)
{
        struct fpga_sbihdr* sbihdr;    
        int ret_code=0;
     
        sbihdr = filep->private_data;
        printk(KERN_ALERT "Length of the Download data =%d, checksum=%x\n", sbihdr->fpos, sbihdr->checksum);    
        kfree(filep->private_data);
        ret_code = wait_for_device_to_configure();

        return  (ret_code);
}
/* ************************************************************************** */



static ssize_t 
fpga_write (struct file* filep, const char* data, size_t count, loff_t* f_pos)
{
        struct fpga_sbihdr* sbihdr = filep->private_data;
        int                 bytes_left=count;
        int                 result;
        char                temp;

        /* Can't seek (pwrite) on fpga.  */
        if (*f_pos != filep->f_pos){
            return (-ESPIPE);
        }    


        /* Check access to the whole are in one go */
        if (!access_ok(VERIFY_READ,(const void*)data, count)){
            return (-EFAULT);
        }


        /*
         * We now lock against writes.
         */
        if (down_interruptible(&fpga_sem))
            return (-ERESTARTSYS);

        if (!sbihdr->fpos){
            result = place_fpga_into_configure_mode();
            if (result)
                return (result);
        }

        while (bytes_left){           
            GET_USER(temp, data);
            if (write_fpga_data_byte(sbihdr, temp)){
                MSG("pos=%d\n", sbihdr->fpos);
                return (-EILSEQ);
            }
            
            sbihdr->fpos++;
            data++;
            bytes_left--;
        }
        up(&fpga_sem);

        return (count);
}
/* ************************************************************************** */


static int 
fpga_ioctl (struct inode *inode, struct file *filep, unsigned int command, unsigned long argAddress)
{
	    struct fpga_sbihdr*  sbihdr = filep->private_data;      
	    void                *arg;	    
	    int                 arg_size ;
	    int                 dir;        
	    int                 rc = 0;	
       
        /*
         * 分離type如果遇到錯誤的cmd, 就直接傳回ENOTTY
         */
        if (_IOC_TYPE(command) != CREATOR_FPGA_IOCTL_MAGIC) return (-ENOTTY);
	
        arg = (void*)argAddress;        
	    arg_size = _IOC_SIZE(command);
	    dir = _IOC_DIR(command);		
        
        switch (command) {    
        case CREATOR_FPGA_DOWNLOAD_CODE : {            
            creator_fpga_download_t info ;          
            
            if (copy_from_user(&info, arg, arg_size)){
			    return  (-EFAULT);
            }            
            
            rc = fpga_download_code(&info, sbihdr);
            break;        
        }    
        default:
            return (-ENOTTY);
        }
        
        return (rc);        
}
/* ************************************************************************** */



static struct file_operations fpga_fops={
    owner:      THIS_MODULE,    
	write:      fpga_write,
	ioctl:      fpga_ioctl,
	open:       fpga_open,
	release:    fpga_release
};
/* ************************************************************************** */



static struct cdev fpga_cdev = {
	.kobj	=	{.name = MODULE_NAME, },
	.owner	=	THIS_MODULE,
};
/* ************************************************************************** */



static int __init 
init_module_drv_fpga (void)
{
        struct  cdev   *pcdev;    
        dev_t   devno;    
        int	    error;                    
             
	    devno = MKDEV(MAJOR_NUM, 0);	    
        if (register_chrdev_region(devno, MAX_MINORS, MODULE_NAME)){    
            printk(KERN_ALERT "%s: can't get major %d\n", MODULE_NAME, MAJOR_NUM);			    
            return (-EBUSY);
        }		    
        pcdev = &fpga_cdev;		    
    	cdev_init(pcdev, &fpga_fops);
	    pcdev->owner = THIS_MODULE;
    
    	
        /* Register lcdtxt as character device */
        error = cdev_add(pcdev, devno, MAX_MINORS);
        if (error) {
            kobject_put(&pcdev->kobj);
            unregister_chrdev_region(devno, MAX_MINORS);

            printk(KERN_ERR "error register %s device\n", MODULE_NAME);
            
            return (-EBUSY);
        }        
        printk(KERN_ALERT "%s: Version : %s %s\n", MODULE_NAME, MODULE_VERSION_INFO, COPYRIGHT);                       
       
        return (0);  
}
/* ************************************************************************** */



static void __exit 
cleanup_module_drv_fpga (void)
{	       
        cdev_del(&fpga_cdev);
       	unregister_chrdev_region(MKDEV(MAJOR_NUM, 0), MAX_MINORS);
        printk(KERN_ALERT "%s: removed\n", MODULE_NAME);	
}	
/* ************************************************************************** */



/* here are the compiler macro for module operation */
module_init(init_module_drv_fpga);
module_exit(cleanup_module_drv_fpga);

MODULE_AUTHOR(MODULE_AUTHOR_STRING);
MODULE_DESCRIPTION(MODULE_DESCRIPTION_STRING);

