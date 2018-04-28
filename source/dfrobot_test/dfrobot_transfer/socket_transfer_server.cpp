/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.12
**  Company: dfrobot
**  File name: socket_transfer_server.cpp
**
**********************************/

#include <stdio.h>
#include <stdint.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>


#include "socket_transfer_server.h"
#include "socket_transfer_client.h"
#include "osal_linux.h"


#define  SERVER_MAX_LISTEN_NUM     20
#define  SERVER_CTRL_READ          mPipe[0]
#define  SERVER_CTRL_WRITE         mPipe[1]

#define  MAIN_BUF_SIZE             		(1*1024*1024)
#define	 MAX_PACKAGE_DATA_SIZE			65000
#define  MAX_PACKAGE_SIZE				(MAX_PACKAGE_DATA_SIZE+12)



#define  CMD_DEL_CLIENT             'd'
#define  CMD_ABRT_ALL               'a'
#define  CMD_STOP_ALL               'e'


struct ServerCtrlMsg
{
    uint32_t code;
    void*    data;
};




TransferServer* TransferServer::Create()
{
	TransferServer* result = new TransferServer();

	if(result && (result->Construct() < 0))
	{
		perror("pipe\n");
		delete result;
		result = NULL;
	}


	return result;
	
}



TransferServer::TransferServer():
	mSrvSockTcp(-1),
	mPortTcp(DFROBOT_SERVER_PORT),
	mSrvThread(-1),
	mThread(NULL),
	mClientMutex(NULL),
	mMutex(NULL),
	mSendNeedWait(true),//设置成发送需要等待
	mRun(false),
	mGenerateSigFlag(GENERATE_IDLE),
	mSig(0)
{
	memset(mPipe, -1, sizeof(mPipe));
	memset(mClientData, 0, sizeof(TransferClient*) * MAX_CLIENT_NUMBER);
	mSend_buf = new uint8_t[MAIN_BUF_SIZE];
	if(mSend_buf == NULL)
	{
		printf("new mSend_buf error\r\n");
		return;
	}
	memset(mSend_buf, 0, MAIN_BUF_SIZE);
	
}

TransferServer::~TransferServer()
{
	delete []mSend_buf;
	if(mSrvSockTcp >= 0)
	{
		close(mSrvSockTcp);
	}
	
	DFROBOT_DELETE(mThread);
	DFROBOT_DELETE(mClientMutex);
	DFROBOT_DELETE(mMutex);

	if(mPipe[0] >= 0)
	{
		close(mPipe[0]);
	}

	if(mPipe[1] >= 0)
	{
		close(mPipe[1]);
	}

	for (int i = 0; i < MAX_CLIENT_NUMBER; ++ i)
	{
    	delete mClientData[i];
  	}
	
}


int TransferServer::CalCrc( unsigned char* buf)
{
	if(buf == NULL)
		return -1;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return -1;
	int i;
	unsigned char crc0,crc1;
	//int len = buf[3]+4;

	int len = ((buf[9] << 16)|(buf[8] << 8)| buf[7]) + 10;

	crc0 = buf[2];
	crc1 = crc0;

	for(i=3; i<len; i++)
	{
		crc0 += buf[i];
		crc1 += crc0;
	}
	
	buf[len++] = crc0;
	buf[len++] = crc1;

	return len; //返回buf 的有效长度
}


bool TransferServer::CheckCrc(unsigned char *buf)
{
	
	if(buf == NULL)
		return false;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return false;
	int i;
	unsigned char crc0,crc1;
	//int len = buf[3]+4;
	int len = ((buf[9] << 16)|(buf[8] << 8)| buf[7]) + 10;

	crc0 = buf[2];
	crc1 = crc0;

	for(i=3; i<len; i++)
	{
		crc0 += buf[i];
		crc1 += crc0;
	}

	if(buf[len++] != crc0)
		return false;
	if(buf[len] != crc1)
		return false;
	return true;
	
}



int TransferServer::Construct()
{
	if(NULL == (mMutex = CMutex::Create()))
	{
		printf("mMutex error\n");
		return -1;
	}
	if(NULL == (mClientMutex = CMutex::Create()))
	{
		printf("mClientMutex error\n");
		return -1;
	}
	
	if(pipe(mPipe) < 0)
	{
		perror("pipe\n");
		return -1;
	}

	return 0;
}



bool TransferServer::DeleteClientSession(TransferClient** data)
{
	ServerCtrlMsg msg;
	msg.code = CMD_DEL_CLIENT;
	msg.data = ((void*)data);
	return (ssize_t)sizeof(msg) == write(SERVER_CTRL_WRITE, &msg, sizeof(msg));
}



uint32_t TransferServer::GetRandomNumber()
{
  struct timeval current = {0};
  gettimeofday(&current, NULL);
  srand(current.tv_usec);
  return (rand() % ((uint32_t)-1));
}

void TransferServer::SetGenerateSigFlag(int status)
{
	if(status == 0)
		mGenerateSigFlag = GENERATE_IDLE;
	else if(status == 1)
		mGenerateSigFlag = GENERATE_START;
	else if(status == 2)
	{
		mGenerateSigFlag = GENERATE_RUNING;
	}
	else if(status == 3)
	{
		mGenerateSigFlag = GENERATE_SET;
	}
}
int TransferServer::GetGenerateSigFlag()
{
	return mGenerateSigFlag;
}


void TransferServer::Package_data(uint8_t *jpeg_data, int len)
{
	int i, package_count,package_size;
	uint8_t *tmp = jpeg_data;
	package_size = len;
	package_count = (len/MAX_PACKAGE_DATA_SIZE)+1;

	

	if(mClientMutex == NULL)//jwy add 2016.05.31
	{
		return ;
	}
	
	mSig++;
	if(mSig > 31)
		mSig = 1;
	
	for(i=0; i<(sizeof(mSendDataSize)/sizeof(unsigned int)); i++)
	{
		mSendDataSize[i] = 0;
	}
	

	if(package_count >= 10)
	{
		printf("package:%d count more than 10\r\n",package_count);
		return;
	}
	for(i=0; i<package_count; i++)
	{
	
		unsigned int temp_size;
		//设置数据大小
		temp_size = (package_size > MAX_PACKAGE_DATA_SIZE) ? MAX_PACKAGE_DATA_SIZE : package_size ;
		mSendDataSize[i] = temp_size + 12;
		package_size -= MAX_PACKAGE_DATA_SIZE;
		//添加包头
		mSend_buf[i*MAX_PACKAGE_SIZE + 0] = 0x55;
		mSend_buf[i*MAX_PACKAGE_SIZE + 1] = 0xaa;
		mSend_buf[i*MAX_PACKAGE_SIZE + 2] = 0x20;
		if(len > MAX_PACKAGE_DATA_SIZE)
		{
			mSend_buf[i*MAX_PACKAGE_SIZE + 3] = (1<<7) | (i<<5)| (mSig & 0x1f);
		}
		else
		{
			mSend_buf[i*MAX_PACKAGE_SIZE + 3] = (mSig & 0x1f);
		}

		mSend_buf[i*MAX_PACKAGE_SIZE + 4] = len & 0xff;
		mSend_buf[i*MAX_PACKAGE_SIZE + 5] = (len >> 8) & 0xff;
		mSend_buf[i*MAX_PACKAGE_SIZE + 6] = (len >> 16) & 0xff;

		mSend_buf[i*MAX_PACKAGE_SIZE + 7] = temp_size & 0xff;
		mSend_buf[i*MAX_PACKAGE_SIZE + 8] = (temp_size >> 8) & 0xff;
		mSend_buf[i*MAX_PACKAGE_SIZE + 9] = (temp_size >> 16) & 0xff; //=0

		memcpy(&mSend_buf[i*MAX_PACKAGE_SIZE+10], &tmp[i*MAX_PACKAGE_DATA_SIZE], temp_size);

		//mSend_buf[i*MAX_PACKAGE_SIZE + temp_size + 10] = 0;
		//mSend_buf[i*MAX_PACKAGE_SIZE + temp_size + 11] = 0;
		int res = CalCrc(&mSend_buf[i*MAX_PACKAGE_SIZE]);

		//printf("---------res=%d\4\n",res);
		
	}

	
	
}



void TransferServer::SendJpegDataToClient()
{
	int i;

	int x, y, x_offset, y_offset;

	if(mGenerateSigFlag == GENERATE_RUNING)
	{
		return;
	}
	
	for(i=0; i<(sizeof(mSendDataSize)/sizeof(unsigned int)); i++)
	{
		if(mSendDataSize[i] > 0)
		{
			SendDataToClient(&mSend_buf[i*MAX_PACKAGE_SIZE],mSendDataSize[i]);
		}
	}
	
}


void TransferServer::SendDataToClient(uint8_t *buf,int len)
{
	//static int count = 0;
	if(mClientMutex == NULL)
	{
		return ;
	}
	
	AUTO_LOCK(mClientMutex);
	
	
	int i;
	int sendRet = -1;
	
	for(i=0; i<MAX_CLIENT_NUMBER; ++i)
	{
		if(mClientData[i])
		{
			if(mClientData[i]->mClientUdpOk == false)
			{
				continue;
			}
			sendRet = sendto(mClientData[i]->mTClientUdp, buf, len, 0, 
								(struct sockaddr*)&(mClientData[i]->mClientUdpAddr), sizeof(mClientData[i]->mClientUdpAddr));

			if(sendRet < 0)
			{
				printf("sever send yuv error:%s, prot %hu\n",
					inet_ntoa(mClientData[i]->mClientUdpAddr.sin_addr),ntohs(mClientData[i]->mClientUdpAddr.sin_port));
			}
			else
			{
				
				//count++;
				//printf("--%d--\n",count);
				usleep(5000);
			}
			
				
			
		}
	}
}


void TransferServer::SendPcmDataToClient(uint8_t *buf, int len)
{
	if(buf == NULL)
	{
		return;
	}
	unsigned char *tmp = buf;
	tmp[0] = 0x55;
	tmp[1] = 0xaa;
	tmp[2] = 0x30;
	tmp[3] = len & 0xff;
	tmp[4] = (len >> 8) & 0xff;
	tmp[5] = (len >> 16) & 0xff;
	tmp[6] = (len >> 24) & 0xff;

	tmp[len + 7] = 0;
	tmp[len + 8] = 0;
	
	SendDataToClient(tmp,(len+9));
	
}


void TransferServer::SendCmdToClient(const char *buf,int len)
{
	if(mClientMutex == NULL)
	{
		return ;
	}
	AUTO_LOCK(mClientMutex);
	int i;
	for(i=0; i<MAX_CLIENT_NUMBER; ++i)
	{
		if(mClientData[i])
		{
			send(mClientData[i]->mTcpSock, buf, len, 0);
			usleep(5000);
		}
	}
}

bool TransferServer::SetupServerSocketTcp(uint16_t port)
{
	bool ret = false;
	int reuse = 1;
	printf("SetupServerSocketTcp start \n");
	struct sockaddr_in serverAddr;
	
	mPortTcp = port;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family      = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port        = htons(mPortTcp);

	if((mSrvSockTcp = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP)) < 0)
	{
		perror("socket\n");
		//exit(errno);
	}
	else
	{
		if(setsockopt(mSrvSockTcp, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0)
		{
			int flag = fcntl(mSrvSockTcp, F_GETFL, 0);
			flag |= O_NONBLOCK;
			if(fcntl(mSrvSockTcp, F_SETFL, flag) != 0)
			{
				perror("fcntl\n");
			}
			else if(bind(mSrvSockTcp, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != 0)
			{
				perror("bind\n");
			}
			else if(listen(mSrvSockTcp, SERVER_MAX_LISTEN_NUM) < 0)
			{
				perror("listen\n");
			}
			else
			{
				ret = true;
			}
		}
		else
		{
			perror("setsockopt\n");
		}
	}

	printf("SetupServerSocketTcp ok \n");
	return ret;
	
}


void TransferServer::AbortClient(TransferClient& client)
{
	client.ClientAbort();
	while(client.mClientThread->IsThreadRunning())
	{
		printf("wait for client %s to exit", client.mClientName);
		usleep(10000);
	}
}



int TransferServer::ServerThread()
{
	int maxfd = -1;
	fd_set allset;
	fd_set fdset;

	
	FD_ZERO(&allset);
	FD_SET(mSrvSockTcp, &allset);
	FD_SET(SERVER_CTRL_READ,&allset);
	maxfd = DFROBOT_MAX(mSrvSockTcp, SERVER_CTRL_READ);
	

	signal(SIGPIPE, SIG_IGN);
	printf("------------serverThread-------------\n");
	while(mRun)
	{
		fdset = allset;
		printf("wait for client connect......\n");
		if(select(maxfd+1, &fdset, NULL, NULL, NULL) > 0)
		{
			
			if(FD_ISSET(SERVER_CTRL_READ, &fdset))
			{
				ServerCtrlMsg msg = {0};
				int readCnt = 0;
				ssize_t readRet = 0;

				do
				{
					readRet= read(SERVER_CTRL_READ, &msg, sizeof(msg));
				}while((++readCnt < 5) && ((readRet == 0) ||
								((readRet < 0) && ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))));

				if(readRet < 0)
				{
					printf("read error11\n");
				}
				else
				{
					if(msg.code == CMD_DEL_CLIENT)
					{
						AUTO_LOCK(mClientMutex);
						TransferClient** client = (TransferClient**)msg.data;
						if(*client)
						{
							printf("server delete client %s",(*client)->mClientName);
							delete *client;
						}
					}
					else if((msg.code == CMD_ABRT_ALL) || (msg.code == CMD_STOP_ALL))
					{
						AUTO_LOCK(mClientMutex);
						mRun = false;
						close(mSrvSockTcp);
						mSrvSockTcp = -1;
						int i;
						for(i=0; i<MAX_CLIENT_NUMBER; ++i)
						{
							if(mClientData[i])
							{
								AbortClient(*mClientData[i]);
								delete mClientData[i];
							}
						}
					}
				}
			}
			else if(FD_ISSET(mSrvSockTcp, &fdset))
			{
#if 0  //test
				printf("client connect\n");
				int new_fd;
				socklen_t len;
				struct sockaddr_in their_addr;
				accept(mSrvSockTcp,(struct sockaddr *)&their_addr,&len);
				printf("server:get connection from %s, prot%d,socked%d\n",
				inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),new_fd);
#endif //test
				
				
#if 1
				AUTO_LOCK(mClientMutex);
				TransferClient** self = NULL;
				int i;
				for(i=0; i<MAX_CLIENT_NUMBER; ++i)
				{
					if(!mClientData[i])
					{
						self = &mClientData[i];
						*self = new TransferClient(this, self, (STREAM_PORT_BASE+i));
						break;
					}
				}
				if(!(*self))
				{
					printf("maximum client number is has reached\n");
				}
				else if(!(*self)->InitClient(mSrvSockTcp))
				{
					//失败的情况稍后处理
					printf(" initClient error\n");
					
					if(*self)
					{
						delete *self;
					}
				}
#endif
				
			}
		}
		else
		{
			//perror("select\n");
			printf("slelect error\n");
			break;
		}
		
	}
	
}

bool TransferServer::StartServerThread()
{
	bool ret = true;


	DFROBOT_DELETE(mThread);

	
	if(NULL == (mThread = CThread::Create("TServer", staticServerThread,this)))
	{
		perror("create TServer---------\n");
		ret = false;
	}
	
	return ret;
}


bool TransferServer::start(uint16_t tcpPort)
{
	if(mRun == false)
	{
		mRun = SetupServerSocketTcp(tcpPort);
		StartServerThread();
	}

	return mRun;
}




