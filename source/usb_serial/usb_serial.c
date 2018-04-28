/*
 *  Copyright (c) 2012-2015 Anyka, Inc
 *  Copyright (c) 2012-2015 luoyongchuang
 * See INSTALL for installation details or manually compile with
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/videodev2.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/input.h>
#include "usb_serial.h"


static int fd;  /* the usb gadget buffer deivce fd */
static T_BOOL Transc_WriteASAFile(unsigned long data[], unsigned long len);
static void usb_anyka_command(unsigned char *scsi_data, unsigned long size);

const static unsigned char anyka_confirm_resp[16] = {
    0x41, 0x4E, 0x59, 0x4B, 0x41, 0x20, 0x44, 0x45, 
    0x53, 0x49, 0x47, 0x4e, 0x45
};//ANYKA DESIGNE



int main()
{
    int length, timeout=0;
    unsigned char buf[26] = "";

    if (access("/usr/modules/g_file_storage.ko", F_OK) != 0)
        return 0;
    
    system("insmod /usr/modules/g_file_storage.ko file=/dev/ram0 removable=1 stall=0");
    
    while(0 != access(AK_USBBURN_NODE, F_OK))
    {
        //printf("access /dev/akudc_usbburn failed, timeout:%d\n",timeout);                         
        if (timeout++ >= 5)
            break;
    }
    
    if (timeout >= 5)
    {
        printf("insmod g_file_storage.ko failed!!!!!!\n");  
        return 0;
    }   
    
        /* open the usb gadget buffer */
    fd = open(AK_USBBURN_NODE, O_RDWR); 
    if (fd < 0) {
        perror("cannot open\n");
        return 0;
    }
        
    while (1)
    {
        /* read the command including its params and the data length */
        if (read(fd, buf, 24) != 24) 
        {
            printf("fatal error: ioctl stall failed");
            while (1)
            ;
        }
            
//      for(i=0;i<25;i++)printf("######buf[%d]:0x%x\n",i,buf[i]);
//          printf("length=0x%x\n",length);                 
            
        length = buf[20] | (buf[21] << 8) | (buf[22] << 16) | (buf[23] << 24);
        buf[16] = '\0';
        printf("length=0x%x\n",length);         
        /* execute the command */

        usb_anyka_command(buf, length);
    }
    close(fd);      
    return 0;
}


static void usb_anyka_command(unsigned char *scsi_data, unsigned long size)
{
    volatile unsigned char cmd;
    unsigned long params[3];
    int ret;
    cmd = *(scsi_data + 1);
    params[0] = (( *(scsi_data + 5) ) 
                | ( *(scsi_data + 6) << 8 ) 
                | ( *(scsi_data + 7) << 16 ) 
                | ( *(scsi_data + 8) << 24 ));
    params[1] = (( *(scsi_data + 9) ) 
                | ( *(scsi_data + 10) << 8 ) 
                | ( *(scsi_data + 11) << 16 ) 
                | ( *(scsi_data + 12) << 24 ));

//  printf("usb_anyka_command####CMD####0x%x:,params=0x%x:\n", cmd,params);
    if (cmd >=0x80)
    {
        cmd -= 0x80;
        switch(cmd)
        {
            case TRANS_WRITE_ASA_FILE:
               ret = Transc_WriteASAFile(params, size);
            break;                                                              
            default:
                printf("comm type get info error\n");
                ret = 0;    
        }       
        if (AK_TRUE == ret) 
        {
            if (ioctl(fd, AK_USBBURN_STATUS, 0) < 0) 
            {
                printf("fatal error: ioctl stall failed");
                while (1);              
            }
            printf("receive usb cmd and data success!!\n");
        }
        else
        {
            printf("\nERRORS!!!!\n");
            if (ioctl(fd, AK_USBBURN_STATUS, 1) < 0) 
            {
                printf("fatal error: ioctl stall failed");
                while (1);                  
            }       
        }                       
    }
    else 
    {   
        switch (cmd)
        {                 
            case ANYKA_UFI_CONFIRM_ID:
                if (ioctl(fd, AK_USBBURN_STALL, 0) < 0)
                {
                    printf("fatal error: ioctl stall failed\n");
                    while (1)
                        ;
                }
                if (write(fd, anyka_confirm_resp, 13) != 13) {
                    printf("write anyka confirm data error\n");
                }
                if (ioctl(fd, AK_USBBURN_STATUS, 0) < 0) {
                    printf("fatal error: ioctl stall failed\n");
                    while (1)
                        ;
                }
                break;
            default:
                printf("end\n");        
        }
    }
    printf("..............................\n");     
}
/*
 * @BREIF    transc of write ASA file
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_WriteASAFile(unsigned long data[], unsigned long len)
{
    char sern_p[249]={0};
    int fd_config;
    
    if (read(fd, sern_p, len) != len) 
    {
        printf("read nand param error");
        return AK_FALSE;
    }   
    
    fd_config = open("/etc/jffs2/tencent.conf", O_RDWR|O_CREAT|O_TRUNC);
    if(fd_config<0)
    {
        printf("open tencentconf file error!!!\n");
        exit(1);
    }
  
    lseek(fd_config,0,SEEK_SET);//将源文件的读写指针移到起始位置  
    
    if (write(fd_config,sern_p,len) != len) 
    {
        printf("write tencentconf file param error");
        close(fd_config);
        return AK_FALSE;
    }   
    close(fd_config);
    return AK_TRUE;
}
