#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#define  CMDSIZE            1024
#define  RECV_BUF_SIZE      1024

int
FindStrFromBuf (const char *buf, int buflen, const char *findingstr,
		int findinglen, int direction)
{
  int i, j;
  if (direction == 0)
    {
      for (i = 0; i < buflen - findinglen + 1; i++)
	{
	  for (j = 0; j < findinglen; j++)
	    {
	      if (buf[i + j] != findingstr[j])
		break;
	      if (j == findinglen - 1)
		return i;
	    }
	}
    }
  else
    {
      for (i = buflen - findinglen; i >= 0; i--)
	{
	  for (j = 0; j < findinglen; j++)
	    {
	      if (buf[i + j] != findingstr[j])
		break;
	      if (j == findinglen - 1)
		return i;
	    }
	}
    }
  return -1;
}

int 
do_command (char *tmpstr, int fd)
{
    printf("do_command:[%s]\r\n",tmpstr);
    system(tmpstr);
    return 0;
}

int 
cmd_listen (int port)
{
  int pos;
  int fd;
  int addrlen;
  int opt = 1;
  int length;
  int clientfd;
  int recv_len = 0;
  char recv_buf[RECV_BUF_SIZE*2];
  char tmpstr[CMDSIZE];
  int boundarylen = strlen ("$$boundary\r\n");
  struct sockaddr_in addr;
  struct sockaddr_in client_addr;

  fd = socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
  if (fd < 0)
    {
      printf ("cmd_listen:socket error.\r\n");
      return -1;
    }
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof (opt));

  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = htonl (INADDR_ANY);
  memset (&(addr.sin_zero), 0, sizeof (addr.sin_zero));
  if (bind (fd, (struct sockaddr *) &addr, sizeof (addr)) != 0)
    {
      printf ("cmd_listen:Binding error.\r\n");
      close (fd);
      return -1;
    }
  if (listen (fd, 100) != 0)
    {
      printf ("cmd_listen:Listen Error.\r\n");
    }
  printf ("cmd_listen:Listen to %d\r\n", port);

  while (1)
    {
      addrlen = sizeof (client_addr);
      clientfd =
	accept (fd, (struct sockaddr *) &client_addr, (int *) &addrlen);
      if (clientfd != -1)
	{
	   printf ("Client %s is connected.\r\n", inet_ntoa (client_addr.sin_addr));
           while(1)
           {
              length =
	         recv (clientfd, recv_buf + recv_len, RECV_BUF_SIZE, 0);
	      if (length <= 0)
	      {
	         close (clientfd);
                 break;
	      }
	      else
	      {
	          recv_len += length;
	          while ((pos =
		      FindStrFromBuf (recv_buf, recv_len, "$$boundary\r\n", boundarylen, 0)) > 0)
		  {
		     memset (tmpstr, 0, CMDSIZE);
		     memcpy (tmpstr,recv_buf, pos);
		     memcpy (recv_buf,recv_buf + pos + boundarylen,recv_len - pos - boundarylen);
		     recv_len = recv_len - pos - boundarylen;
		     do_command (tmpstr, clientfd);
	             close (clientfd);
                     break;
		 }
	     }
           }
	}
    }
  return -1;
}

int 
main(int argc,char *argv[])
{
    signal (SIGPIPE, SIG_IGN);	
    cmd_listen (999) ;
    return 0;
}
