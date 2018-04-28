/*********************************
**  Author : wenyi.jing
**  Date:	2016.4.12
**  Company: dfrobot
**  File name: socket_transfer_server.h
**
**********************************/


#ifndef _SOCKET_TRANSFER_SERVER_H_
#define _SOCKET_TRANSFER_SERVER_H_

#include <stdint.h>
#include <pthread.h>

#include "osal_linux.h"
#include "socket_transfer_client.h"




#define DFROBOT_SERVER_PORT   3232
#define STREAM_PORT_BASE 50000


#define DFROBOT_MAX(_a, _b)			((_a) > (_b) ? (_a) : (_b))



class TransferClient;


class TransferServer
{

	friend class TransferClient;

	enum {
      MAX_CLIENT_NUMBER = 1//32
    };
public:
	TransferServer();
	~TransferServer();

	
public:
	static TransferServer* Create();
	bool start(uint16_t tcpPort = DFROBOT_SERVER_PORT);

	uint32_t GetRandomNumber();
	void SendDataToClient(uint8_t *buf, int len);
	void SendJpegDataToClient();
	void SendPcmDataToClient(uint8_t *buf, int len);
	void Package_data(uint8_t *jpeg_data, int len);
	void SetGenerateSigFlag(int status);
	int GetGenerateSigFlag();
	void SendCmdToClient(const char *buf,int len);
	void GetRectAData(int &width,int &height, int &x_offset, int &y_offset, int &signum)
	{
		width = mWidth;
		height = mHeight;
		x_offset = mXOffset;
		y_offset = mYOffset;
		signum = mGenerateSig_signum;
	}
	
private:
	static int staticServerThread(void *data)
	{
		return ((TransferServer*)data)->ServerThread();
	}
	

private:
	int CalCrc(unsigned char* buf);
	bool CheckCrc(unsigned char *buf);
	bool StartServerThread();
	bool SetupServerSocketTcp(uint16_t port);
	int  ServerThread();
	int  Construct();
	bool DeleteClientSession(TransferClient** data);
	void AbortClient(TransferClient& client);

private:
	int                 mPipe[2];
	uint16_t 			mPortTcp;
	int                 mSrvSockTcp;
	pthread_t 			mSrvThread;
	bool                mRun;
	CThread            	*mThread;
	CMutex             	*mClientMutex;
	CMutex             	*mMutex;
	TransferClient		*mClientData[MAX_CLIENT_NUMBER];
	bool                mSendNeedWait;
	int					mGenerateSigFlag;
	uint8_t 			mSig;
	unsigned int 		mSendDataSize[10];
	uint8_t 			*mSend_buf;
	int					mGenerateSig_signum; 
	int					mWidth;
	int 				mHeight;
	int					mXOffset;
	int 				mYOffset;
};


#endif  //_SOCKET_TRANSFER_SERVER_H_



