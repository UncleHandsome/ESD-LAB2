/*
 * The A2D of the FPGA_EP1C6_LOADER driver for Creator
 *
 *
 * Copyright (C) 2006 by Microtime Computer Inc.
 *
 * Linux kernel version history:
 * Version   : 1.00
 * History
 *   1.0.0 : Programming start 
 *
 */
 
#ifndef _CREATOR_PXA270_FPGA_EP1C6_A2D_H_ 
#define _CREATOR_PXA270_FPGA_EP1C6_A2D_H_ 

#include <linux/config.h>
#if defined(__linux__)
#include <asm/ioctl.h>		/* For _IO* macros */
#define CREATOR_FPGA_IOCTL_NR(n)	     		_IOC_NR(n)
#elif defined(__FreeBSD__)
#include <sys/ioccom.h>
#define CREATOR_FPGA_IOCTL_NR(n)	     		((n) & 0xff)
#endif


#define FPGA_ADC_MAJOR_NUM			    125
#define CREATOR_FPGA_ADC_IOCTL_MAGIC	FPGA_ADC_MAJOR_NUM
#define CREATOR_FPGA_ADC_IO(nr)			_IO(CREATOR_FPGA_ADC_IOCTL_MAGIC,nr)
#define CREATOR_FPGA_ADC_IOR(nr,size)	_IOR(CREATOR_FPGA_ADC_IOCTL_MAGIC,nr,size)
#define CREATOR_FPGA_ADC_IOW(nr,size)	_IOW(CREATOR_FPGA_ADC_IOCTL_MAGIC,nr,size)
#define CREATOR_FPGA_ADC_IOWR(nr,size)	_IOWR(CREATOR_FPGA_ADC_IOCTL_MAGIC,nr,size)


/* Creator FPGA specific ioctls 			*/
#define FPGA_ADC_START_CMD              CREATOR_FPGA_ADC_IO(0x00)
#define FPGA_ADC_STOP_CMD	            CREATOR_FPGA_ADC_IO(0x01)
#define FPGA_ADC_GET_VALUE_CMD			CREATOR_FPGA_ADC_IOR(0x02, unsigned short)

/* A2D status       */
typedef enum {				/* ADC ª¬ºA			 */
    A2D_ON=0, 				/* °_°ÊA2D		   	 */	
    A2D_OFF, 				/* Ãö³¬A2D 		   	 */
} adc_status_e ;	

// function headers


#endif // _CREATOR_PXA270_FPGA_EP1C6_A2D_H_ 

