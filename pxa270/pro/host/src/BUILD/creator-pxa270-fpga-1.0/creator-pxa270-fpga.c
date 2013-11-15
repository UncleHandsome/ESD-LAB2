// --------------------------------------------------------------------
//
//   Title     :  creator-pxa270-fpga.c
//             :
//   Library   :
//             :
//   Developers:  MICROTIME MDS group
//             :
//   Purpose   :  A2D demo for Create FPGA-EP1C6 Program
//             :
//   Limitation:
//             :
//   Note      :
//             :
// --------------------------------------------------------------------
//   modification history :
// --------------------------------------------------------------------
//   Version| mod. date: |
//   V1.00  | 06/16/2006 | First release
// --------------------------------------------------------------------
//
// Note:
//
//       MICROTIME COMPUTER INC.
//
//
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "asm/arch/lib/def.h"
#include "asm/arch/lib/creator_pxa270_fpga_ep1c6_loader.h"
#include "asm/arch/lib/creator_pxa270_fpga_ep1c6_a2d.h"


/*************************************************************************
Variable define
*************************************************************************/


static int
input_timeout (int filedes, unsigned int ms)
{
        fd_set set;
        struct timeval timeout;
        int    socket_id;
       
        /* Initialize the file descriptor set. */
        FD_ZERO (&set);
        FD_SET (filedes, &set);
       
        /* Initialize the timeout data structure. */
        timeout.tv_sec = 0;
        timeout.tv_usec = ms*1000;
       
        socket_id = filedes + 1 ;        
        /* select returns 0 if timeout, 1 if input available, -1 if error. */
        return (select(socket_id, &set, NULL, NULL, &timeout));
}
/* ************************************************************************** */



static int
Process (void)
{
        int		        fd, i, result=0, terminate ;
        unsigned short  a2d_data ;
        unsigned char 	SelectNum;

        fd = open("/dev/fpga_a2d", O_RDWR);
        if (fd < 0){
            printf("open /dev/fpga_adc error\n");
            return (-1);
	    }
	    
	    // start ADC convert
	    result = ioctl(fd, FPGA_ADC_START_CMD, NULL);
	    if (result < 0){
            printf("Start command error for /dev/fpga_adc\n");	        
	        goto out1;
	    }    
	    
	    printf("any key to stop\n");
	    terminate = 1;
        while (terminate){
	        result = ioctl(fd, FPGA_ADC_GET_VALUE_CMD, &a2d_data);
	        if (!result){            
	            printf("A2D data=%02x\r", a2d_data);
	            fflush(stdout);
	        }    
	        
	        if (input_timeout(STDIN_FILENO, 250) > 0){
	            terminate = 0;
	        }    
        }	
        // stop ADC convert
	    result = ioctl(fd, FPGA_ADC_STOP_CMD, NULL);        
        printf("\n\nTerminate A2D convert\n");
        
out1:        
        close(fd);	       
        return (result);
}
/* ************************************************************************** */



static long
file_size (int fd)
{
        off_t len, curpos ;

        curpos = lseek(fd, 0L, SEEK_CUR);
        len = lseek(fd, 0L, SEEK_END);
        lseek(fd, curpos, SEEK_SET);    
        
        return (len);
}  
/* ************************************************************************** */



static int
DownloadFPGA (const char *pDevicenode, char *pDataFileName)
{
        creator_fpga_download_t info ;    
        unsigned long       dwReadBytes;
        int                 fd, result = OK;        
        int                 FPGA_fd ;
        
        FPGA_fd = open(pDevicenode, O_RDWR);
	    if (FPGA_fd < 0){
	        printf("open device %s error\n", pDevicenode);
	        return (-1);
	    }                  

        if ((fd = open(pDataFileName,  O_RDONLY)) < 0){
            printf("open FPAG data file : %s Fail\n", pDataFileName); 
            result = -1;
            goto out2;           
        }            
       
        info.dwSize = file_size(fd);
        info.pBuffer = (char*)malloc(info.dwSize);
        if (info.pBuffer == NULL){
            result = -1;
            goto out1;
        }    

        dwReadBytes = read(fd, info.pBuffer, info.dwSize);
        if (dwReadBytes != info.dwSize){
            printf("Read file : %s Fail\n", pDataFileName);             
            result = -1;
            goto out1 ;
        }
        
        result = ioctl(FPGA_fd, CREATOR_FPGA_DOWNLOAD_CODE, &info); 
        if (!result){
            result = 0;            
        }    
        else{
            printf("Download data to FPGA Fail\n");
            result = -1;
        }   
out1:
        free(info.pBuffer);
        close(fd);        
out2:         
        close(FPGA_fd);    
        return (result);                
}
/* ************************************************************************** */
    


static int  
Initial_FPGA (const char *pDevicenode, char *pDataFileName)
{ 
        int  result ;
        
        result = DownloadFPGA(pDevicenode, pDataFileName);        
        
        return (result);
}  
/* ************************************************************************** */



static void
usage (void)
{
        fflush(stdout);
        fputs("Usage: fpga-creator-pxa270 [-d device_node] -f filename\n", stderr);
        fflush(NULL);
	
        exit(1);
}
/* ************************************************************************** */



int 
main (int argc, char**argv)
{
        char *pDataFileName = NULL, *pDevicenode;    
        int	 ret ;
        
        pDevicenode = "/dev/fpga";
        while ( *++argv ) {		
            if (**argv == '-' ) {
                switch ( argv[0][1] ) {
                case 'h':
                    usage();
                    break;	
                case 'd' :{
                    if (*++argv == NULL) {
                        usage();
                    }
                    pDevicenode = *argv;
		            break;
                }	                	    
                case 'f' :
                    if (*++argv == NULL) {
                        usage();
                    }
                    pDataFileName = *argv;
		            break;
                }		
            }    
        }
        if (!pDataFileName){
            usage();
        }     
       
        printf("device node=%s, DataFile=%s\n", pDevicenode, pDataFileName);
        if (Initial_FPGA(pDevicenode, pDataFileName) < 0){
            return (-1);
        }    
        ret = Process();
        
        return (ret);
}
/* ************************************************************************** */
