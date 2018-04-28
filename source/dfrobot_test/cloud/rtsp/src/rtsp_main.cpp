#include <AKRTSPServer.hh>
#include <BasicUsageEnvironment.hh>
#include <AKIPCH264OnDemandMediaSubsession.hh>
#include <AKIPCH264FramedSource.hh>
#include <AKIPCMJPEGOnDemandMediaSubsession.hh>
#include <AKIPCMJPEGFramedSource.hh>
#include <AKIPCAACAudioOnDemandMediaSubsession.hh>


#ifdef __cplusplus
extern "C" {
#endif

#include <execinfo.h>
#include "common.h"
#include "mid.h"

#ifdef __cplusplus
}
#endif

#define RTSPPORT 554 //rtsp端口号

typedef enum VideoMode_en
{
  VIDEO_MODE_QVGA = 0,
  VIDEO_MODE_VGA,
  VIDEO_MODE_D1,
  VIDEO_MODE_720P,
  VIDEO_MODE_MAX
}VIDEO_MODE;

typedef struct RTSP_CTRL_INFO
{
    int video_vga_run_flag;
    int video_720_run_flag;
    pthread_mutex_t     rtsp_lock;
    sem_t   video_vga_sem;
    sem_t   video_720_sem;
    void *video_vga_queue;
    void *video_720_queue;
}rtsp_ctrl_info, *p_rtsp_ctrl_info;


static p_rtsp_ctrl_info rtsp_ctrl;


UsageEnvironment* env;


/**
* @brief   rtsp_video_move_queue_copy
*       将参数的数据另存为一份新的使用
* @date   2015/4
* @param: T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   T_STM_STRUCT * 
* @retval   返回新的数据的指针
*/

T_STM_STRUCT * rtsp_video_move_queue_copy(T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *new_stream;

    new_stream = (T_STM_STRUCT *)malloc(sizeof(T_STM_STRUCT));
    if(new_stream == NULL)
    {
        return NULL;
    }
    memcpy(new_stream, pstream, sizeof(T_STM_STRUCT));

    new_stream->buf = (T_pDATA)malloc(pstream->size);
    if(new_stream->buf == NULL)
    {
        free(new_stream);
        return NULL;
    }
    memcpy(new_stream->buf, pstream->buf, pstream->size);
    return new_stream;
}


/**
* @brief   rtsp_get_vga_video_data
*       获取vga 数据的callback
* @date   2015/4
* @param: void *param: now no used; T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   void
* @retval   
*/

void rtsp_get_vga_video_data(void *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *pvideo;
    if(pthread_mutex_trylock(&rtsp_ctrl->rtsp_lock) != 0){
        if(errno == EBUSY)
            return;
    }
    pvideo = rtsp_video_move_queue_copy(pstream);
    if(pvideo)
    {
        if(0 == anyka_queue_push(rtsp_ctrl->video_vga_queue, pvideo))
        {
            anyka_free_stream_ram(pvideo);
        }
        else
        {
            sem_post(&rtsp_ctrl->video_vga_sem);
        }
    }
    pthread_mutex_unlock(&rtsp_ctrl->rtsp_lock);
}


/**
* @brief   rtsp_get_720_video_data
*       获取720p 数据的callback
* @date   2015/4
* @param: void *param: now no used; T_STM_STRUCT *pstream: 指向参数结构的指针
* @return   void
* @retval   
*/

void rtsp_get_720_video_data(void *param, T_STM_STRUCT *pstream)
{
    T_STM_STRUCT *pvideo;
    
    if(pthread_mutex_trylock(&rtsp_ctrl->rtsp_lock) != 0){
        if(errno == EBUSY)
            return;
    }
    pvideo = rtsp_video_move_queue_copy(pstream);
    if(pvideo)
    {
        if(0 == anyka_queue_push(rtsp_ctrl->video_720_queue, pvideo))
        {
            anyka_free_stream_ram(pvideo);
        }
        else
        {
            sem_post(&rtsp_ctrl->video_720_sem);
        }
    }
    pthread_mutex_unlock(&rtsp_ctrl->rtsp_lock);
}

/**
* @brief   RTSP_start
*       启动rtsp 预览
* @date   2015/4
* @param: int index: 0-> 启动VGA 编码，1-> 启动编码
* @return   void
* @retval   
*/

void RTSP_start(int index)
{
    pthread_mutex_lock(&rtsp_ctrl->rtsp_lock);
    if(index == 0)
    {
        printf("[%s:%d]#####  Rtsp start  vga  #####\n", __func__, __LINE__);
        rtsp_ctrl->video_vga_queue = anyka_queue_init(150);
        rtsp_ctrl->video_vga_run_flag = 1;
        video_add(rtsp_get_vga_video_data, rtsp_ctrl->video_vga_queue, FRAMES_ENCODE_VGA_NET, 800);
    }
    else
    {
        printf("[%s:%d]#####  Rtsp start  720p  #####\n", __func__, __LINE__);
        rtsp_ctrl->video_720_queue = anyka_queue_init(200);
        rtsp_ctrl->video_720_run_flag = 1;
        video_add(rtsp_get_720_video_data, rtsp_ctrl->video_720_queue, FRAMES_ENCODE_RECORD, 2048);
    }
    
    pthread_mutex_unlock(&rtsp_ctrl->rtsp_lock);
}


/**
* @brief   RTSP_end
*       停止rtsp，回收系统资源
* @date   2015/4
* @param: int index: 0-> 启动VGA 编码，1-> 启动编码
* @return   void
* @retval   
*/

void RTSP_end( int index )
{
    pthread_mutex_lock(&rtsp_ctrl->rtsp_lock);
    if(index == 0)
    { 
        printf("[%s:%d]#####  Rtsp stop  vga  #####\n", __func__, __LINE__);
        rtsp_ctrl->video_vga_run_flag = 0;
        video_del(rtsp_ctrl->video_vga_queue);
        anyka_queue_destroy(rtsp_ctrl->video_vga_queue, anyka_free_stream_ram);
        rtsp_ctrl->video_vga_queue = NULL;
    }
    else
    {
        printf("[%s:%d]#####  Rtsp stop 720p  #####\n", __func__, __LINE__);
        rtsp_ctrl->video_720_run_flag = 0;
        video_del(rtsp_ctrl->video_720_queue);
        anyka_queue_destroy(rtsp_ctrl->video_720_queue, anyka_free_stream_ram);
        rtsp_ctrl->video_720_queue = NULL;
    }
    pthread_mutex_unlock(&rtsp_ctrl->rtsp_lock);
}


/**
* @brief   RTSP_get_video_data
*       获取当前帧视频数据的信息
* @date   2015/4
* @param: void* buf, 存放当前帧的数据
      unsigned* nlen, 存放当前帧数据长度
      int nNeedIFrame, 外部需要的I 帧数目
      struct timeval* ptv, 存放时间信息结构体
      int index, 0-> VAG 数据，1-> 720P 数据
* @return   int
* @retval   0-> success, -1 -> failed
*/

int RTSP_get_video_data(void* buf, unsigned* nlen, int nNeedIFrame, struct timeval* ptv, int index)
{
    T_STM_STRUCT *pvideo = NULL;
    if(index == 0)
    {
        if(rtsp_ctrl->video_vga_run_flag)
        {
            while((pvideo = (T_STM_STRUCT *)anyka_queue_pop(rtsp_ctrl->video_vga_queue)) == NULL)
            {
                sem_wait(&rtsp_ctrl->video_vga_sem);
            }
        }
    }
    else
    {
        if(rtsp_ctrl->video_720_run_flag)
        {
            while((pvideo = (T_STM_STRUCT *)anyka_queue_pop(rtsp_ctrl->video_720_queue)) == NULL)
            {
                sem_wait(&rtsp_ctrl->video_720_sem);
            }
        }
    }    
    if(nNeedIFrame && pvideo->iFrame == 0)
    {
        anyka_free_stream_ram(pvideo);
        return -1;
    }
    *nlen = pvideo->size;
    memcpy(buf, pvideo->buf, *nlen);
    ptv->tv_sec = pvideo->timestamp / 1000;
    ptv->tv_usec = (pvideo->timestamp % 1000) * 1000;
    anyka_free_stream_ram(pvideo);

  return 0;
}


/**
* @brief   RTSP_init
*       初始化RTSP 功能，注册callback等
* @date   2015/4
* @param: void
* @return   void
* @retval   
*/

void RTSP_init(void)
{
    rtsp_ctrl = (p_rtsp_ctrl_info)malloc(sizeof(rtsp_ctrl_info));
    if(NULL == rtsp_ctrl)
    {
        return ;
    }
    memset(rtsp_ctrl, 0, sizeof(rtsp_ctrl_info));
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    UserAuthenticationDatabase* authDB = NULL;
    #ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
    #endif
    
    // Create the RTSP server:
    RTSPServer* rtspServer = AKRTSPServer::createNew(*env, RTSPPORT, authDB);
    if (rtspServer == NULL) 
    {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }
    
    char const* descriptionString = "Session streamed by \"testOnDemandRTSPServer\"";
    
    // Set up each of the possible streams that can be served by the
    // RTSP server.  Each such stream is implemented using a
    // "ServerMediaSession" object, plus one or more
    // "ServerMediaSubsession" objects for each audio/video substream.
    
    int vsIndex = 0;
    VIDEO_MODE vm[2] = {VIDEO_MODE_VGA,VIDEO_MODE_720P};
	
    const char* streamName1 = "ch0_1.264";
    const char* streamName2 = "ch0_0.264";
    ((AKRTSPServer*)rtspServer)->SetStreamName(streamName1, streamName2); 
    
    vm[0] = VIDEO_MODE_VGA;
    
    AKIPCH264FramedSource* ipcSourcecam0 = NULL;
    ServerMediaSession* smscam0 = ServerMediaSession::createNew(*env, streamName1, 0, descriptionString);
    AKIPCH264OnDemandMediaSubsession* subscam0 = AKIPCH264OnDemandMediaSubsession::createNew(*env, ipcSourcecam0, 0, vsIndex);
    smscam0->addSubsession(subscam0);
    subscam0->getframefunc = RTSP_get_video_data;
    subscam0->setledstart = RTSP_start;
    subscam0->setledexit = RTSP_end;
    
    rtspServer->addServerMediaSession(smscam0);
    char* url1 = rtspServer->rtspURL(smscam0);
    *env << "using url for vga  \"" << url1 <<"\"\n";
    delete[] url1;
    
    vsIndex = 1;
    
    vm[1] = VIDEO_MODE_720P;
    
    AKIPCH264FramedSource* ipcSourcecam = NULL;
    ServerMediaSession* smscam = ServerMediaSession::createNew(*env, streamName2, 0, descriptionString);
    AKIPCH264OnDemandMediaSubsession* subscam = AKIPCH264OnDemandMediaSubsession::createNew(*env, ipcSourcecam, 0, vsIndex);
    smscam->addSubsession(subscam);
    subscam->getframefunc = RTSP_get_video_data;
    subscam->setledstart = RTSP_start;
    subscam->setledexit = RTSP_end;
    
    rtspServer->addServerMediaSession(smscam);
    char* url2 = rtspServer->rtspURL(smscam);
    *env << "using url for 720p \"" << url2 <<"\"\n";
    delete[] url2;
    
    env->taskScheduler().doEventLoop();
    

}

