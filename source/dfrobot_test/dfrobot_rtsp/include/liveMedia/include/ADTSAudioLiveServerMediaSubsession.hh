/*
** filename :ADTSAudioLiveServerMediaSubsession.hh
** author: jingwenyi
**
*/
#ifndef _ADTS_AUDIO_LIVE_SERVER_MEDIA_SUBSESSION_HH_
#define  _ADTS_AUDIO_LIVE_SERVER_MEDIA_SUBSESSION_HH_

#include "OnDemandServerMediaSubsession.hh"
#include "ADTSAudioLiveSource.hh"

class ADTSAudioLiveSource;

typedef void (*ADTSCallBackFun) (ADTSAudioLiveSource* subsms);

class ADTSAudioLiveServerMediaSubsession: public OnDemandServerMediaSubsession
{
	//friend class ADTSAudioLiveSource;
public: 
	static ADTSAudioLiveServerMediaSubsession* 
		createNew(UsageEnvironment& env, Boolean reuseFirstSource, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel);
protected:
	ADTSAudioLiveServerMediaSubsession(UsageEnvironment& env, Boolean reuseFirstSource, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel);
	virtual ~ADTSAudioLiveServerMediaSubsession();
protected:
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned & estBitrate);
	virtual void closeStreamSource(FramedSource* inputSource);
	virtual RTPSink* createNewRTPSink(Groupsock * rtpGroupsock,unsigned char rtpPayloadTypeIfDynamic,FramedSource * inputSource);

public:
	
	
	
private:
	u_int8_t 		profile;
	unsigned int 	samplingFrequency;
	u_int8_t 		channel;
public:
	ADTSCallBackFun CallBackSubSms;
	ADTSCallBackFun CallBackSubSmsClose;
	
};



#endif  //_ADTS_AUDIO_LIVE_SERVER_MEDIA_SUBSESSION_HH_
