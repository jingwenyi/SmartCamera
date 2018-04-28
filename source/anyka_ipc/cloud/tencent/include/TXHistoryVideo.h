#ifndef __TX_HISTORY_VIDEO_H__
#define __TX_HISTORY_VIDEO_H__

#include "TXSDKCommonDef.h"
#include "TXMsg.h"
#include "TXDataPoint.h"

enum historyVideoPropertyId
{
	DPPropertyId_getVideoRangeList = 200019,
	DPPropertyId_startHistoryVideo = 200020,
};

enum historyVideoList
{
	COUNT = 1,
	RANGE_LIST = 2,
	START_TIME = 3,
	END_TIME = 4,
};


CXX_EXTERN_BEGIN

/**
 * 视频回看的历史视频区间
 */
typedef struct _tx_history_video_range
{
	unsigned int start_time;
	unsigned int end_time;
	unsigned long long frame_count;
} tx_history_video_range;

typedef struct _tx_history_video_notify
{
	void (*on_query_history_video)(unsigned int lastTime, unsigned int maxCount, unsigned int *count, tx_history_video_range * rangelist);
//	void (*on_start_history_video)(unsigned int playTime);
	void (*on_start_history_video)(unsigned int playTime, unsigned long long base_time);
	
} tx_history_video_notify;

void onReplyVideoListResult(int err_code, unsigned long long to_client, const char *api_name, int api_len, tx_api_data_point data_points[], int data_points_count, unsigned int cookie);
bool onQueryHistoryVideo(const tx_ccmsg_inst_info * from_client, const char * api_name, int api_len, const tx_api_data_point * data_point);
bool onStartHistoryVideo(const tx_ccmsg_inst_info * from_client, const char * api_name, int api_len, const tx_api_data_point * data_point);

SDK_API void tx_init_history_video_notify(tx_history_video_notify *notify);

CXX_EXTERN_END

#endif
