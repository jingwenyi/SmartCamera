/*
** filename:ADTSAudioLiveSource.hh
** author: jingwenyi
** date : 2016.6.14
**
*/

#ifndef  _ADTS_AUDIO_LIVE_SOURCE_HH_
#define  _ADTS_AUDIO_LIVE_SOURCE_HH_

#include "FramedSource.hh"
#include "ADTSAudioLiveServerMediaSubsession.hh"


typedef int (*callBackFun)(unsigned char* buffer, int len);


class ADTSAudioLiveSource: public FramedSource
{
	//friend class ADTSAudioLiveServerMediaSubsession;
public:
	static ADTSAudioLiveSource* createNew(UsageEnvironment& env, u_int8_t profile, 
									unsigned int samplingFrequency, u_int8_t channel);

	unsigned samplingFrequency() const { return fSamplingFrequency; }
	unsigned numChannels() const { return fNumChannels; }
	char const* configStr() const { return fConfigStr; }

public:
	 callBackFun	getAudioData;  //get audio data call back function
	
private:
  ADTSAudioLiveSource(UsageEnvironment& env, u_int8_t profile,
		      u_int8_t samplingFrequencyIndex, u_int8_t channelConfiguration);
	// called only by createNew()

  virtual ~ADTSAudioLiveSource();

private:

	// redefined virtual functions:
  virtual void doGetNextFrame();

	

private:
  unsigned fSamplingFrequency;
  unsigned fNumChannels;
  unsigned fuSecsPerFrame;
  char fConfigStr[5];
};



#endif //_ADTS_AUDIO_LIVE_SOURCE_HH_