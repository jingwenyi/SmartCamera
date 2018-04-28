/**********************************
**
** 	Author: 		wenyi.jing  
** 	Date:	    	13/04/2016
** 	Company:	dfrobot
**	File name: 	osal_linux.cpp
**
************************************/

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "osal_linux.h"




#define gettid() syscall(__NR_gettid)



CMutex* CMutex::Create(bool bRecursive)
{
  CMutex *result = new CMutex();
  if (result && result->Construct(bRecursive) != 0) {
    delete result;
    result = NULL;
  }
  return result;
}


int CMutex::Construct(bool bRecursive)
{
  if (bRecursive) {
    pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&mMutex, &attr);
  } else {
    // a littler faster than recursive
    ::pthread_mutex_init(&mMutex, NULL);
  }
  return 0;
}



CThread* CThread::Create(const char *pName, DFROBOT_THREAD_FUNC entry, void *pParam)
{
  CThread *result = new CThread(pName);
  if (result && result->Construct(entry, pParam) != 0) {
    delete result;
    result = NULL;
  }
  return result;
}


CThread::CThread(const char *pName) :
    mbThreadCreated(false),
    mbThreadRunning(false),
    mThread(0),
    mpName(pName),
    mEntry(NULL),
    mpParam(NULL)
{
}

int CThread::Construct(DFROBOT_THREAD_FUNC entry, void *pParam)
{
  mEntry = entry;
  mpParam = pParam;
  mbThreadRunning = false;

  if (DFROBOT_UNLIKELY(0 != pthread_create(&mThread,
                                      NULL,
                                      __Entry,
                                      (void*) this))){
    printf("thread construct error\n");                    	
    return -1;
  }
#if 0 //jwy add 2016.4.20  编译器不支持pthread_setname_np 这个函数
  if (DFROBOT_UNLIKELY(pthread_setname_np(mThread, mpName) < 0)) {
    perror("Failed to set thread name:");
  }
#endif 
  mbThreadCreated = true;
  return 0;
}

CThread::~CThread()
{
  if (mbThreadCreated) {
    pthread_join(mThread, NULL);
  }
}


void *CThread::__Entry(void *p)
{
  CThread *pthis = (CThread *) p;

#ifdef AM_DEBUG
  pthread_setspecific(g_thread_name_key, pthis);
  signal(SIGSEGV, segfault_handler);
#endif

  printf("thread %s created, tid %ld\n", pthis->mpName, gettid());

  pthis->mbThreadRunning = true;
  pthis->mEntry(pthis->mpParam);
  pthis->mbThreadRunning = false;

  printf("thread %s exits\n", pthis->mpName);
  return NULL;
}


//CEvent
CEvent* CEvent::Create()
{
  CEvent* result = new CEvent();
  if (result && result->Construct() != 0) {
    delete result;
    result = NULL;
  }
  return result;
}

int CEvent::Construct()
{
  sem_init(&mEvent, 0, 0);
  return 0;
}

void CEvent::Delete()
{
  delete this;
}







