/**********************************
**
** 	Author: 	wenyi.jing  
** 	Date:	    	13/04/2016
** 	Company:	dfrobot
** 	File name:	osal_linux.h
**
************************************/
#ifndef _OSAL_LINUX_H_
#define _OSAL_LINUX_H_

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>



#define DFROBOT_LIKELY(x)   (__builtin_expect(!!(x),1))
#define DFROBOT_UNLIKELY(x) (__builtin_expect(!!(x),0))


#define DFROBOT_DELETE(_obj) \
  do { if (_obj) (_obj)->Delete(); _obj = NULL; } while (0)

class CThread;
class CMutex;
class CAutoLock;
class CCondition;
class CEvent;





/********************************
**		CMutex class
**
*********************************/

class CMutex
{
	friend class CCondition;
	
public:
	static CMutex* Create(bool bRecursive = true);
	void Delete()
	{
		delete this;
	}
	
public:
	void Lock()
	{
		pthread_mutex_lock(&mMutex);
	}
	void Unlock()
	{
		pthread_mutex_unlock(&mMutex);
	}
	bool Trylock()
	{
		return (0 == pthread_mutex_trylock(&mMutex));
	}

private:
	CMutex()
	{
	}
	int Construct(bool bRecursive);
	~CMutex()
	{
	  pthread_mutex_destroy(&mMutex);
	}
	
private:
	pthread_mutex_t mMutex;
};



class CAutoLock
{
  public:
    CAutoLock(CMutex *pMutex) :
        _pMutex(pMutex)
    {
      _pMutex->Lock();
    }
    ~CAutoLock()
    {
      _pMutex->Unlock();
    }

  private:
    CMutex *_pMutex;
};

#define AUTO_LOCK(pMutex)      CAutoLock __lock__(pMutex)
#define __LOCK(pMutex)         pMutex->Lock()
#define __UNLOCK(pMutex)       pMutex->Unlock()
#define __TRYLOCK(pMutex)      pMutex->Trylock()



/********************************
**		CThread class
**
*********************************/
typedef int (*DFROBOT_THREAD_FUNC)(void*);
class CThread
{
  public:
    static CThread* Create(const char *pName,
                           DFROBOT_THREAD_FUNC entry,
                           void *pParam);
    void Delete()
    {
      delete this;
    }

  public:
    enum
    {
      PRIO_LOW     = 70,
      PRIO_NORMAL  = 80,
      PRIO_HIGH    = 90,
      PRIO_HIGHEST = 99
    };
    //int SetRTPriority(int priority = PRIO_HIGHEST);
    bool IsThreadRunning() {return mbThreadRunning;}
    //static CThread* GetCurrent();
    //static void DumpStack();
    const char *Name()
    {
      return mpName;
    }

  private:
    CThread(const char *pName);
    int Construct(DFROBOT_THREAD_FUNC entry, void *pParam);
    ~CThread();

  private:
    static void *__Entry(void*);

  private:
    bool mbThreadCreated;
    bool mbThreadRunning;
    pthread_t mThread;
    const char *mpName;

    DFROBOT_THREAD_FUNC mEntry;
    void *mpParam;
};



/**************************
**
**  CCondition
**
*************************/
class CCondition
{
		//friend class CQueue;
	
public:
	static CCondition* Create()
	{
		 return new CCondition();
	}
	void Delete()
	{
		 delete this;
	}
	
public:
	void Wait(CMutex *pMutex)
	{
#ifdef AM_DEBUG
	AM_ASSERT(0 == pthread_cond_wait(&mCond, &pMutex->mMutex));
#else
	pthread_cond_wait(&mCond, &pMutex->mMutex);
#endif
	}
	
	void Signal()
	{
#ifdef AM_DEBUG
		AM_ASSERT(0 == pthread_cond_signal(&mCond));
#else
		pthread_cond_signal(&mCond);
#endif
	}
	
	void SignalAll()
	{
#ifdef AM_DEBUG
		 AM_ASSERT(0 == pthread_cond_broadcast(&mCond));
#else
		 pthread_cond_broadcast(&mCond);
#endif
	}
	
private:
	CCondition()
	{
#ifdef AM_DEBUG
		AM_ASSERT(0 == pthread_cond_init(&mCond, NULL));
#else
		pthread_cond_init(&mCond, NULL);
#endif
	}
	~CCondition()
	{
		pthread_cond_destroy(&mCond);
	}
	
private:
	pthread_cond_t mCond;
};

/****************************
**
**  CEvent
**
*********************************/
class CEvent
{
public:
	static CEvent* Create();
	void Delete();
	
public:
	int Wait(int64_t ms = -1)
	{
		if (ms < 0) {
			sem_wait(&mEvent);
			return 0;
		} else {
			struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_sec += ms / 1000;
			ts.tv_nsec += (ms % 1000) * 1000000;
			return sem_timedwait(&mEvent, &ts) == 0 ? 0 : -1;
		 }
	}
	
	void Signal()
	{
		 sem_trywait(&mEvent);
		 sem_post(&mEvent);
	}
	
	void Clear()
	{
		 sem_trywait(&mEvent);
	}
	
private:
	CEvent()
	{
	}
	int Construct();
	~CEvent()
	{
		 sem_destroy(&mEvent);
	}
	
private:
	sem_t mEvent;
};




#endif  //_OSAL_LINUX_H_


