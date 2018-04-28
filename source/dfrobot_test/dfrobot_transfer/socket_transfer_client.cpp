/**********************************
**
** 	Author: 		wenyi.jing  
** 	Date:	    	13/04/2016
** 	Company:	dfrobot
**	File name: 	socket_transfer_client.cpp
**
************************************/


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/file.h>
#include <json/json.h> //add for json
#include "json/json_api.h"



#include "socket_transfer_client.h"
#include "socket_transfer_server.h"
#include "dfrobot_main.h"
#include "base64.h"

#include "interface.h"


static char need_aplay_mp3_buffer[200];



using namespace std;


#define  SEND_BUF_SIZE             (1*1024*1024)
#define	 RECV_MAX_SIZE			   (100*1024)
#define  SOCK_TIMEOUT_SECOND       5

//dfrobot multicast address 224.2.2.2
#define	 DFROBOT_MULTICAST    "224.2.2.2"
#define	 DFROBOT_MULTICAST_PORT		55353
#define	 DFROBOT_MULTICAST_TEST


#define  CMD_CLIENT_ABORT          'a'
#define  CMD_CLIENT_STOP           'e'


//jwy add 2016.4.18 for cmd 

#define		CMD_SERVER_UDPPORT			0X01
#define		CMD_CLIENT_UDPADDR			0X02
#define		CMD_SERVER_UDPDATA_YUV		0X10
#define		CMD_SERVER_UDPDATA_JPEG		0X20

//add end






TransferClient::TransferClient(TransferServer* server, TransferClient**location, uint16_t Port):
	mTServer(server),
	mSelfLocation(location),
	mClientSessionId(0),
	mClientName(NULL),
	mTcpSock(-1),
	mTClientUdp(-1),
	mClientUdpOk(false),
	mClientThread(NULL),
	mSessionState(CLIENT_SESSION_INIT),
	mDynamicTimeOutSec(CLIENT_TIMEOUT_THRESHOLD),
	mUdpPort(Port)
{
	memset(&mClientAddr, 0, sizeof(mClientAddr));
	memset(&mClientUdpAddr, 0, sizeof(mClientUdpAddr));
}
TransferClient::~TransferClient()
{
	*mSelfLocation = NULL;

	if(mClientCtrlSock[0] >= 0)
	{
		close(mClientCtrlSock[0]);
	}

	if(mClientCtrlSock[1] >= 0)
	{
		close(mClientCtrlSock[1]);
	}
	
	DFROBOT_DELETE(mClientThread);
	CloseAllSocket();
	delete[] mClientName;
	
}


void TransferClient::CloseAllSocket()
{
	if(mTcpSock >= 0)
	{
		close(mTcpSock);
		mTcpSock = -1;
	}
	if(mTClientUdp >= 0)
	{
		close(mTClientUdp);
		mTClientUdp = -1;
	}
}



int TransferClient::ClientThread()
{
	bool run = true;
	int maxfd = -1;

	int readRet = 0;
	int j = 0;

	fd_set allset;
	fd_set fdset;

	timeval timeout = {CLIENT_TIMEOUT_THRESHOLD,0};
	
	FD_ZERO(&allset);
	FD_SET(mTcpSock, &allset);
	FD_SET(mTClientUdp, &allset);
	FD_SET(CLIENT_CTRL_READ, &allset);
	maxfd = DFROBOT_MAX(mTcpSock, mTClientUdp);
	maxfd = DFROBOT_MAX(maxfd, CLIENT_CTRL_READ);

	printf("new client:%s, prot %hu\n", inet_ntoa(mClientAddr.sin_addr),ntohs(mClientAddr.sin_port));

	signal(SIGPIPE, SIG_IGN);


	while(run)
	{
		int retval = -1;
		timeval* tm = &timeout;
		timeout.tv_sec = mDynamicTimeOutSec;

		fdset = allset;
		if(retval = select(maxfd+1, &fdset, NULL, NULL, tm) > 0)
		{
			if(FD_ISSET(CLIENT_CTRL_READ, &fdset))
			{
				char cmd[1] = {0};
				int readCnt = 0;
				ssize_t readRet = 0;
				do
				{
					readRet = read(CLIENT_CTRL_READ, &cmd, sizeof(cmd));
				}while((++readCnt < 5) && ((readRet == 0)|| ((readRet < 0) && 
						((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)))));
				if(readRet <= 0)
				{
					printf("read error22\n");
				}
				else if((cmd[0] == CMD_CLIENT_STOP) || (cmd[0] == CMD_CLIENT_ABORT))
				{
					printf("client:%s exit", mClientName);
					run = false;
					CloseAllSocket();
					break;
				}
				
			}
			else if(FD_ISSET(mTcpSock, &fdset))
			{
				//接收从客服端发过来的消息并处理回复消息
#if 0
				int len;
				static char buf[RECV_MAX_SIZE] = {0};
				

				memset(buf, 0, RECV_MAX_SIZE);

				do
				{
					readRet = recv(mTcpSock, buf, RECV_MAX_SIZE, 0);
				}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));

#endif
				
				int readCnt = 0;
				//read header
				unsigned char header[2] = {0};
				do
				{
					readCnt = 0;
					do
					{
						readRet = recv(mTcpSock, &header[0], 1, 0);
					}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));
				}while(0);


				if(readRet > 0)
				{
					//check header
					do
					{
						if(0x55 != header[0]) continue;
						readCnt = 0;
						do
						{
							readRet = recv(mTcpSock, &header[1], 1, 0);
						}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));

						if(readRet <= 0)
						{
							goto close_socket;
						}


						if(0xaa != header[1]) continue;
					}while(0);
					
					//read json size
					unsigned char json_data_len[4] = {0};
					do
					{
						readCnt = 0;
						do
						{
							readRet = recv(mTcpSock, json_data_len, 4, 0);
						}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));
					
						if(readRet <= 0)
						{
							goto close_socket;
						}

						if((readRet != 4))
						{
							printf("readRet < 4\r\n");
							continue;
						}
					}while(0);

					unsigned int data_len = 0;
					data_len = (json_data_len[3] << 24) | (json_data_len[2] << 16) | (json_data_len[1] << 8) | json_data_len[0];
					printf("json data len:%d\r\n", data_len);

					char *json_data = (char*)malloc(data_len+1);
					if(json_data == NULL)
					{
						printf("json data malloc error\r\n");
						continue;
					}
					memset(json_data, 0, data_len+1);

					//read json data
					int jj=0;
					{
						readCnt = 0;
						do
						{
							readRet = recv(mTcpSock, &json_data[jj], data_len-jj, 0);
							jj += readRet;
						}while((++readCnt < 5) && ((jj< data_len) || (readRet == 0) || (readRet < 0) && (errno == EINTR)));

						if(readRet <= 0)
						{
							goto close_socket;
						}
					}

					if(jj != data_len)
					{
						printf("jj=%d, json_data_len=%d\r\n", jj, data_len);
						
						free(json_data);
						continue;
					}

					//printf("recive json:\r\n");
					//printf("%s\r\n",json_data);

					

					//handle data
					Json::Reader reader;
					Json::Value value;
					
					reader.parse(json_data, value);

					if(value.isMember("Cmd") == false)
					{
						free(json_data);
						continue;
					}
					
					

					printf("value:%s\r\n",value["Cmd"].asCString());
#if 1  //ClientVoice

					if(!strcmp(value["Cmd"].asCString(), "ClientVoice"))
					{
						
						int totallen = value["TotalLen"].asInt();
						int total_package = value["TotalPack"].asInt();
						int current_package = value["CurrentPack"].asInt();
						int len = 0, size = 0; 
						char *c_value;
						unsigned char *tmp_data = NULL;
						char *tmp_base64_data = NULL;
						printf("totallen=%d, total package=%d, currentpackage=%d\r\n", totallen, total_package,current_package);
						int err_flag = 0;
												
						time_t now = time(NULL);
						struct tm *timeinfo = localtime(&now);
						strftime(need_aplay_mp3_buffer, sizeof(need_aplay_mp3_buffer),"/tmp/%Y_%m_%d_%H_%s.mp3",timeinfo);
						FILE *fd = fopen(need_aplay_mp3_buffer, "wb+");
						
						{
							std::string c_value = value["Data"].asCString();
							size = c_value.size();
							
							len = value["CurrentLen"].asInt();
							
							tmp_base64_data = (char*)malloc(size+1);
							if(tmp_base64_data == NULL)
							{
								printf("tmp base64 data malloc failed\r\n");
								err_flag = 1;
								free(tmp_data);
								goto finish;
							}
							memset(tmp_base64_data, 0, size+1);
							sprintf(tmp_base64_data, "%s", c_value.c_str());
						
													
							//printf("size=%d,c_value:%s\r\n",size, tmp_base64_data);
							int return_len = 0;
							tmp_data = base64_decode(tmp_base64_data, size, &return_len);
							printf("len=%d,return_len=%d\r\n", len, return_len);
							if(tmp_data == NULL)
							{
								printf("tmp data malloc failed\r\n");
								err_flag = 1;
								goto finish;
							}
												
							fwrite(tmp_data, 1, len, fd);
							free(tmp_data);
							tmp_data =	NULL;
							free(tmp_base64_data);
							tmp_base64_data = NULL;
							free(json_data);
							json_data = NULL;
						}


						for(j=2; j<=total_package; j++)
						{
							//get header
							memset(header, 0, sizeof(header));
							readCnt = 0;
							do
							{
								readRet = recv(mTcpSock, &header[0], 2, 0);
							}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));
							
							if((readRet <= 0) || (0x55 != header[0]) || (0xaa != header[1]))
							{
								printf("readRet=%d, header[0]=%x, header[1]=%x\r\n", readRet,header[0], header[1]);
								err_flag = 1;
								break;
							}
							

							//get data len
							memset(json_data_len, 0, sizeof(json_data_len));
							readCnt = 0;
							do
							{
								readRet = recv(mTcpSock, json_data_len, 4, 0);
							}while((++readCnt < 5) && ((readRet == 0) || (readRet < 0) && (errno == EINTR)));

							if((readRet <= 0) || (readRet != 4))
							{
									err_flag = 1;
									break;
							}
							data_len = (json_data_len[3] << 24) | (json_data_len[2] << 16) | (json_data_len[1] << 8) | json_data_len[0];
							printf("json data len:%d\r\n", data_len);


							//get json data
							json_data = (char*)malloc(data_len+1);
							if(json_data == NULL)
							{
								printf("json data malloc error\r\n");
								err_flag = 1;
								break;
							}
							memset(json_data, 0, data_len+1);
							
							jj=0;
							{
								readCnt = 0;
								do
								{
									readRet = recv(mTcpSock, &json_data[jj], data_len-jj, 0);
									jj += readRet;
								}while((++readCnt < 5) && ((jj< data_len) || (readRet == 0) || (readRet < 0) && (errno == EINTR)));
							
								if(readRet <= 0)
								{
									err_flag = 1;
									break;
								}
							}
							
							if(jj != data_len)
							{
								printf("jj=%d, json_data_len=%d\r\n", jj, data_len);
													
								free(json_data);
								err_flag = 1;
								break;
							}
							
							//printf("package=%d,recive json:\r\n",j);
							//printf("%s\r\n",json_data);

							//handle json data
							reader.parse(json_data, value);
							
							if((0 != strcmp(value["Cmd"].asCString(), "ClientVoice")) || (j != value["CurrentPack"].asInt()))
							{
								printf("-------cmd=%s,currentPack=%d-,j=%d--------\r\n",value["Cmd"].asCString(), value["CurrentPack"].asInt(),j);
								err_flag = 1;
								break;
							}

							{
								std::string c_value = value["Data"].asCString();
								size = c_value.size();
	
								len = value["CurrentLen"].asInt();
								
								tmp_base64_data = (char*)malloc(size+1);
								if(tmp_base64_data == NULL)
								{
									printf("tmp base64 data malloc failed\r\n");
									err_flag = 1;
									break;
								}
								memset(tmp_base64_data, 0, size+1);
								sprintf(tmp_base64_data, "%s", c_value.c_str());

							
								//printf("size=%d,c_value:%s\r\n",size, tmp_base64_data);

						
								int return_len = 0;
								tmp_data = base64_decode(tmp_base64_data, size, &return_len);
								printf("len=%d,return_len=%d\r\n", len, return_len);
								if(tmp_data == NULL)
								{
									printf("tmp data malloc failed\r\n");
									err_flag = 1;
									break;
								}
						
								fwrite(tmp_data, 1, len, fd);
								free(tmp_data);
								tmp_data = 	NULL;
								free(tmp_base64_data);
								tmp_base64_data = NULL;
								free(json_data);
								json_data = NULL;
							}
							
						}


finish:
										
						fclose(fd);
						if(tmp_data != NULL)
						{
							free(tmp_data);
						}
						if(json_data != NULL)
						{
							free(json_data);
						}

						if(tmp_base64_data != NULL)
						{
							free(tmp_base64_data);
						}
						
						if(err_flag == 1)
						{
							err_flag = 0;
							printf("----------------------error------------------\r\n");
							char buffer[250];
							sprintf(buffer, "rm -rf %s",need_aplay_mp3_buffer);
							system(buffer);
							continue;
						}
										
						pc_voice_mp3_notify(need_aplay_mp3_buffer);
						
					}
					else
#endif //ClientVoice
					if(!strcmp(value["Cmd"].asCString(), "GetResolution"))
					{
						Json::Value item;
						int w = 0, h = 0;
						unsigned char current_resolution = get_current_resolution();

						switch(current_resolution)
						{
							case R1280X720:
								w = 1280;
								h = 720;
								break;
							case R640X480:
								w = 640;
								h = 480;
								break;
							case R320X240:
								w = 320;
								h = 240;
								break;
							default:
								break;
						}
						
						item["cmd"] = "CurrentResolution";
						item["Width"] = w;
						item["Height"] = h;

						
						std::string sendString = item.toStyledString();
						
						//printf("%s\r\n",sendString.c_str());
						send(mTcpSock, sendString.c_str(), sendString.size(), 0);
	

					}
					else if(!strcmp(value["Cmd"].asCString(), "SetResolution"))
					{
						int w = 0, h = 0;
						bool flag = false;
						bool switch_flag = true;
						Json::Value item;
						unsigned char current_resolution;
						
						w = value["Width"].asInt();
						h = value["Height"].asInt();

						current_resolution = get_current_resolution();

						if((w == 1280) && (h == 720))
						{
							if(current_resolution == R1280X720)
								switch_flag = false;
							set_current_resolution(R1280X720);
							flag = true;
							
						}
						else if((w == 640) && (h == 480))
						{
							if(current_resolution == R640X480)
								switch_flag = false;
							set_current_resolution(R640X480);
							flag = true;
							
						}
						else if((w == 320) && (h == 240))
						{
							//no support
							flag = false;
						}
						else
						{
							flag = false;
						}

					

						item["cmd"] = "RespondSetResolution";
						if(!flag)
						{
							item["Status"] = "Failed";
							std::string sendString = item.toStyledString();
						
							//printf("%s\r\n",sendString.c_str());
							send(mTcpSock, sendString.c_str(), sendString.size(), 0);
							continue;
						}

						if(switch_flag)
							switch_resolution(w, h);

						{
							item["Status"] = "Ok";
							std::string sendString = item.toStyledString();
						
							//printf("%s\r\n",sendString.c_str());
							int ret = send(mTcpSock, sendString.c_str(), sendString.size(), 0);
							//printf("-----ret=%d-------\r\n",ret);

						}
					}
					else if(!strcmp(value["Cmd"].asCString(), "GetVoiceSize"))
					{
						Json::Value item;
						int voice_size = (int)get_current_voice_size();
						
						item["cmd"] = "CurrentVoiceSize";
						item["VoiceSize"] = voice_size;
						std::string sendString = item.toStyledString();
						
						//printf("%s\r\n",sendString.c_str());
						send(mTcpSock, sendString.c_str(), sendString.size(), 0);
						
					}
					else if(!strcmp(value["Cmd"].asCString(), "ChangeVoiceSize"))
					{
						int voice_size = 0;
						Json::Value item;

						voice_size = value["VoiceSize"].asInt();
						if(voice_size >= 5)
							voice_size = 5;
						if(voice_size <= 0)
							voice_size = 0;
						set_current_voice_size((unsigned char)voice_size);

						item["cmd"] = "CurrentVoiceSize";
						item["VoiceSize"] = voice_size;

						std::string sendString = item.toStyledString();
						
						//printf("%s\r\n",sendString.c_str());
						send(mTcpSock, sendString.c_str(), sendString.size(), 0);
						
					}
					else if(!strcmp(value["Cmd"].asCString(), "ChangeName"))
					{
						const char* tmp = value["HostName"].asCString();
						Json::Value item;
						
						main_set_config_name(tmp);

						item["cmd"] = "RespondChangeName";
						item["Status"] = "Ok";

						std::string sendString = item.toStyledString();
						
						//printf("%s\r\n",sendString.c_str());
						send(mTcpSock, sendString.c_str(), sendString.size(), 0);
						
					}
					else if(!strcmp(value["Cmd"].asCString(), "SetBaiduTuling"))
					{
						TULING_SET_PARAM tmp_param;
						bool res = false;
						Json::Value item;

						tmp_param.baidu_user_id = (char *)value["BaiduUserId"].asCString();
						tmp_param.baidu_api_key = (char *)value["BaiduApiKey"].asCString();
						tmp_param.baidu_api_sercret = (char *)value["BaiduSecret"].asCString();
						tmp_param.tuling_user_id = (char *)value["TulingUserId"].asCString();
						tmp_param.tuling_api_key = (char *)value["TulingApiKey"].asCString();

						res = set_tuling_param(&tmp_param);

						item["cmd"] = "RespondSetBaiduTuling";

						if(!res)
						{
							item["Status"] = "Failed";
						}
						else
						{
							item["Status"] = "Ok";
						}
						
						std::string sendString = item.toStyledString();
						
						//printf("%s\r\n",sendString.c_str());
						send(mTcpSock, sendString.c_str(), sendString.size(), 0);
						
					}
					else if(!strcmp(value["Cmd"].asCString(), "HeartBeat"))
					{
						//heartbeat package
						//printf("--------------heartbeat\r\n");
					}

				}
				else if(readRet <= 0)
				{
close_socket:
					if((errno == ECONNRESET) || (readRet == 0))
					{
						if(mTcpSock >= 0)
						{
							close(mTcpSock);
							mTcpSock = -1;
						}
						printf("close mTcpsock\n");
					}
					else
					{
						printf("recv error\n");
					}
					run = false;
					continue;
				}	
			

			
			}

			else if(FD_ISSET(mTClientUdp, &fdset))
			{
			//do nothing
#if 0
				unsigned char buf[50];
				int len;
				memset(buf, 0, sizeof(buf));
				struct sockaddr_in clientAddr;
				socklen_t addrLen = sizeof(clientAddr);
				memcpy(&clientAddr, &mClientAddr, sizeof(clientAddr));
				clientAddr.sin_port = htons(mUdpPort);
				if((len = recvfrom(mTClientUdp, buf, sizeof(buf), 0,(struct sockaddr*)&clientAddr, &addrLen)) < 0)
				{
					printf("recvfrom error\n");
				}
				else
				{
					if(len < 6)
					{
						printf("len < 6 :%d,buf:%s\r\n",len,buf);
						continue;
					}
					else if(CheckCrc(buf) == false)
					{
						printf("server udp data check error,buf:%s\n",buf);
						continue;
					}
					else if(buf[2] == CMD_CLIENT_UDPADDR)
					{
						if(mClientUdpOk == false)
						{
						#ifndef  DFROBOT_MULTICAST_TEST  //modify multicast
							memcpy(&mClientUdpAddr, &clientAddr, sizeof(clientAddr));
							mClientUdpOk= true;
						#endif
						}
					#if 0
						printf("rcv msg client udpaddr\n");
						if(sendto(mTClientUdp, "i am server udp-----", 100, 0,
									(struct sockaddr*)&mClientUdpAddr, sizeof(mClientUdpAddr)) < 0)
						{
							printf(" server udp sendto error\n");
						}
					#endif
					}
				
				}
#endif
				

#if 0 //test
				//test udp
				char testbuf[100] = {0};
				int ret = -1;
				struct sockaddr_in clientAddr;
				socklen_t addrLen = sizeof(clientAddr);
				memcpy(&clientAddr, &mClientAddr, sizeof(clientAddr));
				clientAddr.sin_port = htons(mUdpPort);

				if((ret = recvfrom(mTClientUdp, testbuf, sizeof(testbuf), 0,
										(struct sockaddr*)&clientAddr, &addrLen)) < 0)
				{
					printf("udp recvfrom error\n");
				}
				else
				{
					//udp 数据不处理，值打印
					printf("udp rec msg:%x\n",testbuf[2]);
					//printf("clientaddr:%s, clientaddr:%s\n", inet_ntoa(clientAddr.sin_addr),
													//		   inet_ntoa(clientAddr.sin_addr));
					printf("client udp --------:%s, prot %hu\n", inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port));
					if(sendto(mTClientUdp, "i am server udp", 100, 0,
									(struct sockaddr*)&clientAddr, sizeof(clientAddr)) < 0)
					{
						printf(" server udp sendto error\n");
					}
				}
#endif
				
			}
			
			
		}
		else if(retval == 0)
		{
			printf("client: %s is not responding within %d seconds, shutdown!\n",mClientName,mDynamicTimeOutSec);
			run = false;
			//CloseAllSocket();
			
		}
		else
		{
			perror("select client\n");
			run = false;
		}
	}

	mSessionState = CLIENT_SESSION_STOPPED;
	

	if(!run)
	{
		int count = 0;
		bool val = false;
		while((++count < 5)&& !(val = mTServer->DeleteClientSession(mSelfLocation)))
		{
			usleep(5000);
		}

		if((count >= 5) && !val)
		{
			printf("TServer is dead before client session exit\n");
		}
		
	}
	
	
	return 0;
}

void TransferClient::ClientAbort()
{
	while(mSessionState == CLIENT_SESSION_INIT)
	{
		usleep(100000);
	}

	if((mSessionState == CLIENT_SESSION_OK) || (mSessionState == CLIENT_SESSION_THREAD_RUN))
	{
		char cmd[1] = {CMD_CLIENT_ABORT};
		int count = 0;

		while((++count < 5) && (1 != write(CLIENT_CTRL_WRITE, cmd, 1)))
		{
			if((errno != EAGAIN) && (errno != EWOULDBLOCK) && (errno != EINTR))
			{
				perror("write\n");
				break;
			}
		}
		printf("abort command to client:%s\n",mClientName);
	}
	else
	{
		printf("this client is not initialized successfully!\n");
	}
	
	
}


int TransferClient::CalCrc( unsigned char* buf)
{
	if(buf == NULL)
		return -1;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return -1;
	int i;
	unsigned char crc0,crc1;
	int len = buf[3]+4;

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


bool TransferClient::CheckCrc(unsigned char *buf)
{
	
	if(buf == NULL)
		return false;
	if((0x55 != buf[0]) || (0xaa != buf[1]))
		return false;
	int i;
	unsigned char crc0,crc1;
	int len = buf[3]+4;

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


int TransferClient::SetupServerSocketUdp(uint16_t streamPort)
{
	int reuse = 1;
	int sock = -1;
	 struct sockaddr_in streamAddr;

	 printf("server udp port:%d\n",streamPort);

	 memset(&streamAddr, 0, sizeof(streamAddr));
	 streamAddr.sin_family = AF_INET;
#ifndef  DFROBOT_MULTICAST_TEST 
	 streamAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	 streamAddr.sin_port = htons(streamPort);
#else //udp multcast 
	 streamAddr.sin_addr.s_addr = inet_addr(DFROBOT_MULTICAST);
	 streamAddr.sin_port = htons(DFROBOT_MULTICAST_PORT);
	 memcpy(&mClientUdpAddr, &streamAddr, sizeof(streamAddr));
	 mClientUdpOk= true;
#endif

	//printf("----------SetupServerSocketUdp------------\n");

	 if((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	 {
		perror("socket client\n");
	 }
	 else
	 {
		int sendBuf = SEND_BUF_SIZE;
		if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0) &&
			(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sendBuf, sizeof(sendBuf))== 0))
		{
			if(bind(sock, (struct sockaddr*)&streamAddr, sizeof(streamAddr)) != 0)
			{
				close(sock);
				sock = -1;
				perror("bind client\n");
			}
			else
			{
				//printf("=====================================\n");
				if(mTServer->mSendNeedWait)// go here
				{
					timeval timeout = {SOCK_TIMEOUT_SECOND*3, 0};
					if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) != 0)
					{
						close(sock);
						sock = -1;
						printf("setsockopt client\n");
					}
					else if(setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) != 0)
					{
						close(sock);
						sock = -1;
						printf("setsockopt client\n");
					}
					//printf("---------create udp ok------------\r\n");
				}
				else
				{
					int flag = fcntl(sock, F_GETFL, 0);
					flag |= O_NONBLOCK;
					if(fcntl(sock, F_SETFL, flag) != 0)
					{
						close(sock);
						sock = -1;
						perror("fcntl client\n");
					}
				}
			}
		}
		else
		{
			close(sock);
			sock = -1;
			perror("setsockopt client\n");
		}
		
	 }
	printf("udp multicast address:%s,port:%d\r\n",DFROBOT_MULTICAST,DFROBOT_MULTICAST_PORT);
	 return sock;
}


bool TransferClient::InitClient(int serverTcpSock)
{
	bool ret = false;
	
	mSessionState = CLIENT_SESSION_INIT;
	
	if(pipe(mClientCtrlSock) < 0)
	{
		perror("client pipe\n");
		mSessionState = CLIENT_SESSION_FAILED;
	}
	else if(mTServer && mSelfLocation && (serverTcpSock >=0))
	{
		unsigned int acceptCnt = 0;
		socklen_t sockLen = sizeof(mClientAddr);
		//产生一个sessionId
		mClientSessionId = mTServer->GetRandomNumber();

		do{
			//连接的 tcp 端口
			mTcpSock = accept(serverTcpSock, (sockaddr*)&(mClientAddr), &sockLen);
		}while((++acceptCnt < 5) && (mTcpSock < 0) && 
				((errno == EAGAIN)|| (errno == EWOULDBLOCK) || (errno == EINTR)));

		printf("mTcpsock = %d\n",mTcpSock);
		if(mTcpSock >= 0)
		{
			//这里只建立一个UDP 服务和建立连接的client 通信
			//这里领导要求用组播
			mTClientUdp = SetupServerSocketUdp(mUdpPort);
		}
		else
		{
			perror("accept\n");
		}

		printf("mTcpsock=%d,mTclientUdp=%d\n",mTcpSock,mTClientUdp);
		if((mTcpSock >= 0) && (mTClientUdp >= 0))
		{
			int	sendBuf = SEND_BUF_SIZE;
			int noDelay = 1;
			timeval timeout = {SOCK_TIMEOUT_SECOND, 0};

			if(setsockopt(mTcpSock, IPPROTO_TCP, TCP_NODELAY,
								&noDelay, sizeof(noDelay)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_SNDBUF,
									&sendBuf, sizeof(sendBuf)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_RCVTIMEO,
									(char*)&timeout, sizeof(timeout)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else if(setsockopt(mTcpSock, SOL_SOCKET, SO_SNDTIMEO,
									(char*)&timeout, sizeof(timeout)) != 0)
			{
				perror("setsockopt\n");
				mSessionState = CLIENT_SESSION_FAILED;
			}
			else
			{
				char clientName[128] = {0};
				sprintf(clientName, "%s-%hu",inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
				SetClientName(clientName);
				mClientThread = CThread::Create(mClientName, StaticClientThread, this);

				if(!mClientThread)
				{
					printf("failed to create client thread for %s\n",clientName);
					mSessionState = CLIENT_SESSION_FAILED;
				}
				else
				{
					ret = true;
					printf("----------client session ok----------\r\n");
					mSessionState = CLIENT_SESSION_THREAD_RUN;
					mSessionState = CLIENT_SESSION_OK;
				}
				
				printf("+++++++++++++++++++++++\r\n");
				
			}
			
		}
		
		
	}
	else //失败打印出原因
	{
		if(!mTServer)
		{
			printf("invalid tcp server\n");
		}
		if(!mSelfLocation)
		{
			printf("invalid location\n");
		}
		if(serverTcpSock < 0)
		{
			printf("invalid tcp sever socket\n");
		}
		mSessionState = CLIENT_SESSION_FAILED;
	}
	
	return ret;
}




