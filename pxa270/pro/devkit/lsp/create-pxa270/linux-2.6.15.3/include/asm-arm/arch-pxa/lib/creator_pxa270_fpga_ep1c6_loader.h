/*
 * FPGA_EP1C6_LOADER driver for Creator
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
 
#ifndef _CREATOR_PXA270_FPGA_EP1C6_LOADER_H_ 
#define _CREATOR_PXA270_FPGA_EP1C6_LOADER_H_ 

#include <linux/config.h>
#if defined(__linux__)
#include <asm/ioctl.h>		/* For _IO* macros */
#define CREATOR_FPGA_IOCTL_NR(n)	     		_IOC_NR(n)
#elif defined(__FreeBSD__)
#include <sys/ioccom.h>
#define CREATOR_FPGA_IOCTL_NR(n)	     		((n) & 0xff)
#endif


#define FPGA_MAJOR_NUM			        124
#define CREATOR_FPGA_IOCTL_MAGIC		FPGA_MAJOR_NUM
#define CREATOR_FPGA_IO(nr)			    _IO(CREATOR_FPGA_IOCTL_MAGIC,nr)
#define CREATOR_FPGA_IOR(nr,size)		_IOR(CREATOR_FPGA_IOCTL_MAGIC,nr,size)
#define CREATOR_FPGA_IOW(nr,size)		_IOW(CREATOR_FPGA_IOCTL_MAGIC,nr,size)
#define CREATOR_FPGA_IOWR(nr,size)		_IOWR(CREATOR_FPGA_IOCTL_MAGIC,nr,size)

typedef struct CREATOR_FPGA_DOWNLOAD {
    unsigned long  dwSize ;
    char           *pBuffer ;
} creator_fpga_download_t ;



/* Creator FPGA specific ioctls 			*/
#define CREATOR_FPGA_DOWNLOAD_CODE      CREATOR_FPGA_IOW(0x00, creator_fpga_download_t)

// function headers


#endif // _CREATOR_PXA270_FPGA_EP1C6_LOADER_H_ 

