// --------------------------------------------------------------------
//
//   Title     :  creator-pxa270-fpga-ep1c6-a2d.c
//             :
//   Library   :
//             :
//   Developers:  MICROTIME MDS group
//             :
//   Purpose   :  A2D Driver for FPGA-EP1C6
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
#include "asm/arch/lib/creator_pxa270_fpga_ep1c6_a2d.h"


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


/* ****** Module Informatio ******************************************** */
#define	MAJOR_NUM			FPGA_ADC_MAJOR_NUM
#define MAX_MINORS          1

#define MODULE_VERSION_INFO	"1.00"
#define MODULE_NAME			"A2D for FPGA-EP1C6"
#define COPYRIGHT			"Copyright (C) 2006 Microtime Computer Inc."
#define MODULE_AUTHOR_STRING		"Microtime Computer Inc."
#define MODULE_DESCRIPTION_STRING	"A2D module"


/* ****** IRQ information ********************************************* */
#define MODULE_IRQ_NO   CREATOR_IO_XIRQ3_EXT_SLAVE_IRQ


/* ****** FPGA information ********************************************* */
#define FPGA_IO_BASE		ECS3_BASE
#define FPGA_RAM_BASE		RCS3_BASE
#define FPGA_CFG_IO_BASE	(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0FF0<<1)))
#define FPGA_CFG_IO_MODE	(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0FF1<<1)))

#define REG_CTRL1			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0000<<1)))
		//00 	: L3C			: R/W-(1)
		//01 	: L3D			: R/W-(1)
		//02 	: L3M			: R/W-(1)
		//03 	: ENTXINT		: R/W-(0)	: Eanble UART TX interrupt
		//04 	: ENRXINT		: R/W-(0)	: Eanble UART RX interrupt
		//05-07 : Reserved		: R/W-(1)
#define REG_CTRL2			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0001<<1)))
		//A2D_CLK_SEL
#define REG_CTRL3			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0002<<1)))
		//7-Segment
#define ADDER   			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0056<<1)))
#define PS2_DATA			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0100<<1)))
#define A2D_DATA			(*(volatile unsigned short *)(FPGA_IO_BASE+(0x0200<<1)))
		
		
/* ****** FPGA run mode ************************************************ */	
#define LAB_MODE_NG			0
#define LAB_MODE_PS2		1
#define LAB_MODE_A2D		2
#define LAB_MODE_I2S_PLAY	3
#define LAB_MODE_I2S_RECORD	4
#define LAB_MODE_UART		5
	


struct  creator_fpga_adc_sbihdr{
	unsigned int fpos;
	unsigned int temp;
};



/*************************************************************************
Function prototypes
*************************************************************************/



/*************************************************************************
Variable define
*************************************************************************/
static DECLARE_MUTEX(creator_fpga_adc_sem);
static const char* g_module_irq_id = "ADC of the FPGA for Creator "; /* global id for irq */
static int   nSlaveIrqNo ;
static u32   A2DStatus;
static UC    a2d_data;


static const UC LED_tbl[] = {
		0xc0, 0xf9, 0xa4, 0xb0, 0x99,
		0x92, 0x82, 0xf8, 0x80, 0x90,
		0xa0, 0x83, 0xc6, 0xa1, 0x84,
	0x8e, 0x7f, 0xff
		};
/* ************************************************************************** */		
		
		
		
static void 
SetFPGAMode (u32 mode)
{
		FPGA_CFG_IO_MODE = mode;      
}    		
/* ************************************************************************** */



static irqreturn_t 
creator_fpga_adc_IRQ (int irq, void *dev_id, struct pt_regs *regs) 
{    
		UN_CVT cvt;    

		a2d_data = (UC)A2D_DATA;

		cvt.b[3] = (UC)(a2d_data & 0xf);
		cvt.b[2] = (UC)((a2d_data>>4) & 0xf);
		cvt.b[0] = 0x11;	//blank
		cvt.b[1] = 0x11;	//blank
		
		REG_CTRL3 = LED_tbl[cvt.b[2]];  

        return (IRQ_HANDLED);
}
/* ************************************************************************** */



static int 
creator_fpga_adc_IRQ_REQUEST (struct creator_fpga_adc_sbihdr*  sbihdr)
{
        int  result = 0;
    
        if (A2DStatus == A2D_OFF){
            nSlaveIrqNo = creator_get_irq_extirq3(MODULE_IRQ_NO);              
            result = request_irq(nSlaveIrqNo, creator_fpga_adc_IRQ, 0, g_module_irq_id, NULL);
            if (result < 0){
                printk(KERN_ALERT "request_irq %x, and  function fail\n\r", nSlaveIrqNo);
                return (result);
            }
        }
        else{ 
            enable_irq(nSlaveIrqNo);         
        }      
        return (result);
}
/* ************************************************************************** */



static void 
creator_fpga_adc_IRQ_FREE (void)
{
        if (A2DStatus == A2D_ON){
            //disable IRQ
            disable_irq(nSlaveIrqNo);
            //free interrupt
            free_irq(nSlaveIrqNo, NULL);    	
        }
}
/* ************************************************************************** */



static int 
creator_fpga_adc_Start (struct creator_fpga_adc_sbihdr*  sbihdr)
{
        if (A2DStatus == A2D_ON){
            return (0);
        }    
            
           
        if (creator_fpga_adc_IRQ_REQUEST(sbihdr) < 0){
            printk(KERN_ALERT "ADC Alloc IRQ Error\n");
            return (-EBUSY);
        }
		FPGA_CFG_IO_BASE = 8;
		SetFPGAMode(LAB_MODE_A2D);        
        A2DStatus = A2D_ON;        
        return (0);
}
/* ************************************************************************** */



static int 
creator_fpga_adc_Stop (struct creator_fpga_adc_sbihdr*  sbihdr)
{      
        if (A2DStatus == A2D_ON){
            creator_fpga_adc_IRQ_FREE();                            
        } 
        A2DStatus = A2D_OFF;
		SetFPGAMode(LAB_MODE_NG);          
        return (0);          
}
/* ************************************************************************** */
   
   
   
static int 
creator_fpga_adc_open (struct inode* inode, struct file *filep)
{
        struct creator_fpga_adc_sbihdr* sbihdr;


        /* Create our private data and link it to the file structure */
        sbihdr = kmalloc(sizeof(struct creator_fpga_adc_sbihdr), GFP_KERNEL);

        if (!sbihdr)
            return -ENOMEM;

        filep->private_data = sbihdr;

        sbihdr->fpos = 0;
        sbihdr->temp = 0;
        
        return (0);
}
/* ************************************************************************** */



static int 
creator_fpga_adc_release (struct inode* inode, struct file *filep)
{
        struct creator_fpga_adc_sbihdr* sbihdr;    
        int ret_code = 0;
     
        sbihdr = filep->private_data; 
        creator_fpga_adc_Stop(sbihdr);        
        kfree(filep->private_data);
        return  (ret_code);
}
/* ************************************************************************** */



static int 
creator_fpga_adc_ioctl (struct inode *inode, struct file *filep, unsigned int command, unsigned long argAddress)
{
	    struct creator_fpga_adc_sbihdr*  sbihdr = filep->private_data;      
	    void                *arg;	    
	    int                 arg_size ;
	    int                 dir;        
	    int                 rc = 0;	
       
        /*
         * 分離type如果遇到錯誤的cmd, 就直接傳回ENOTTY
         */
        if (_IOC_TYPE(command) != CREATOR_FPGA_ADC_IOCTL_MAGIC) return (-ENOTTY);
	
        arg = (void*)argAddress;        
	    arg_size = _IOC_SIZE(command);
	    dir = _IOC_DIR(command);		
        
        switch (command) {    
        case FPGA_ADC_START_CMD : {   
            rc = creator_fpga_adc_Start(sbihdr);           
            break;
        }    
        case FPGA_ADC_STOP_CMD : {   
            rc = creator_fpga_adc_Stop(sbihdr);                          
            break;
        }      
        case FPGA_ADC_GET_VALUE_CMD : {
            unsigned short data ;		
            
              
            data = a2d_data ;  
            if (copy_to_user(arg, &data, arg_size))	             
                return (-EINVAL);
            break;            
        }                 
        default:
            return (-ENOTTY);
        }
        
        return (rc);        
}
/* ************************************************************************** */



static struct file_operations creator_fpga_adc_fops={
    owner:      THIS_MODULE,    
	ioctl:      creator_fpga_adc_ioctl,
	open:       creator_fpga_adc_open,
	release:    creator_fpga_adc_release
};
/* ************************************************************************** */



static struct cdev creator_fpga_adc_cdev = {
	.kobj	=	{.name = MODULE_NAME, },
	.owner	=	THIS_MODULE,
};
/* ************************************************************************** */



static int __init 
init_module_drv_fpga_a2d (void)
{
        struct  cdev   *pcdev;    
        dev_t   devno;    
        int	    error;                    
          
          
        A2DStatus = A2D_OFF ;  
	    devno = MKDEV(MAJOR_NUM, 0);	    
        if (register_chrdev_region(devno, MAX_MINORS, MODULE_NAME)){    
            printk(KERN_ALERT "%s: can't get major %d\n", MODULE_NAME, MAJOR_NUM);			    
            return (-EBUSY);
        }		    
        pcdev = &creator_fpga_adc_cdev;		    
    	cdev_init(pcdev, &creator_fpga_adc_fops);
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
cleanup_module_drv_fpga_a2d (void)
{	       
        creator_fpga_adc_IRQ_FREE();    
        cdev_del(&creator_fpga_adc_cdev);
       	unregister_chrdev_region(MKDEV(MAJOR_NUM, 0), MAX_MINORS);
        printk(KERN_ALERT "%s: removed\n", MODULE_NAME);	
}	
/* ************************************************************************** */



/* here are the compiler macro for module operation */
module_init(init_module_drv_fpga_a2d);
module_exit(cleanup_module_drv_fpga_a2d);

MODULE_AUTHOR(MODULE_AUTHOR_STRING);
MODULE_DESCRIPTION(MODULE_DESCRIPTION_STRING);

