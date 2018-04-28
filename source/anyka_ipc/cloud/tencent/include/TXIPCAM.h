#ifndef __IPCAMERA_H__
#define __IPCAMERA_H__

#include "TXSDKCommonDef.h"
#include "TXDeviceSDK.h"

CXX_EXTERN_BEGIN


/////////////////////////////////////////////////////////////////
//
//         ipcamera互联相关接口: 清晰度调整，云台控制
//
/////////////////////////////////////////////////////////////////


// 视频清晰度
enum definition {
	def_low		= 1,	//低
	def_middle	= 2,	//中
	def_high	= 3,	//高
};

// 云台(PTZ)旋转方向
enum rotate_direction {
	rotate_direction_left	= 1,	//左
	rotate_direction_right	= 2,	//右
	rotate_direction_up		= 3,	//上
	rotate_direction_down	= 4,	//下
};

// 云台(PTZ)旋转角度范围
enum rotate_degree {
	//水平角度范围
	rotate_degree_h_min = 0,
	rotate_degree_h_max = 360,

	//垂直角度范围
	rotate_degree_v_min = 0,
	rotate_degree_v_max = 180,
};

// ipcamera交互接口：清晰度调整，交互
typedef struct _tx_ipcamera_notify {
	/**
	 * 接口说明:	ipcamera清晰度调整回调，调用后返回摄像头的当前分辨率
	 * param : definition :  要调整的清晰度，具体取值参见 enum definition
	 * param : cur_definition：暂时保留，没有用到
	 * param : cur_definition_length : 暂时保留，没有用到。
	*/
	int (*on_set_definition)(int definition, char *cur_definition, int cur_definition_length);

	/**
	 * 接口说明：ipcamera云台控制回调
	 * param ：rotate_direction : 云台调整方向，具体取值可以参见 enum rotate_direction
	 * param ：rotate_degree ： 云台调整角度，具体取值可以参见 enum rotate_degree
	 * return : 0代表调整失败， 非0代表调整成功。
	*/
	int (*on_control_rotate)(int rotate_direction, int rotate_degree);
} tx_ipcamera_notify;


/**
 * 接口说明： 设置ipcamera相关的回调
 * param: notify IPCamera交互回调对象。
 */
SDK_API int tx_ipcamera_set_callback(tx_ipcamera_notify *notify);



/////////////////////////////////////////////////////////////////
//
//         ipcamera互联相关接口: 历史视频回看
//
/////////////////////////////////////////////////////////////////

/**
  *  QQ物联历史视频回看方案：
  *
  *      |-----------|                                        |—————————————|
  *      |           |    <---- 1.on_fetch_history_video --   |             |
  *      |  DEVICE   |                                        |     APP     |
  *      |           |    <---- 2.on_play_history_video ----  |             |
  *      |           |                                        |             |
  *      |-----------|                                        |—————————————|
  *
  *    重要：如果需要使用历史视频回看功能，必须在iot后台开启“是否支持历史视频回看”功能（功能id：200021），否则手机QQ不会显示历史视频的时间轴
  *
  *    step1. 手机QQ在视频连接成功之后向设备查询历史视频纪录用来绘制历史视频的时间轴，所以需要设备本地存储历史视频的文件和文件对应的时间段
  *    step2. 用户在手机QQ上选择历史视频，手机QQ向设备发送播放历史视频命令，视频数据仍然通过tx_set_video_data/tx_set_audio_data
  *           发送，只需切换数据源到历史视频即可。
  *
  *           注意：tx_set_video_data方法的nTimeStamps参数是以 "毫秒" 为单位的,而且是一个相对时间,这个时间是相对于base_time(单位是ms)。
  *           base_time 是通过on_play_history_video接口获取。
  *
  *  手Q中历史视频时间轴示意图:
  *           历史视频片段2                历史视频片段1                  实时视频
  *      ｜----------------------------------------------------------------|
  *      ｜   ||||||||||||||              ||||||||||||||                   |
  *      ｜   ||||||||||||||              ||||||||||||||                   |
  *      ｜----------------------------------------------------------------|
  *           10:30     11:00             12:00     12:30               14:00
  */
  
/**
*  sdk1.3 接口变更说明：
*  为了在32位时间戳字段中传输毫秒级时间1.3引入了base_time字段，填充历史视频数据时使用当前帧的毫秒级时间戳减去base_time得到时间偏移，
*  填入tx_set_video_data接口的nTimeStamps字段中，手机QQ中会根据base_time还原真实的毫秒级时间。主要是解决由于时间精度不够而引起的卡顿问题。
*/


/**
 * 历史视频片段的时间区间
 */
typedef struct _tx_history_video_range
{
	unsigned int start_time;     //视频片段开始时间
	unsigned int end_time;       //视频片段结束时间
} tx_history_video_range;

typedef struct _tx_history_video_notify
{
	/**
	 * 接口说明: 查询历史视频信息 step1
	 * param: last_time   手机QQ分段拉取历史视频信息，last_time为时间轴末端的时间(unix时间戳），例如时间轴示意图中的2:30
	 * param: max_count   请求的时间区间数目
	 * param: count       实际返回的区间数目
	 * param: range_list  sdk动态分配的内存空间，接口实现者需要把区间数据写入其中，由sdk负责释放。一定要和实际返回的数目count匹配，否则会出现未知错误
	 */
	void (*on_fetch_history_video)(unsigned int last_time, int max_count, int *count, tx_history_video_range * range_list);

	/**
	 * 接口说明: 播放历史视频 step2
	 * param: play_time  单位是秒，历史视频开始播放的起始时间(unix时间戳)。其中0表示从历史视频播放切换到实时视频播放
	 * param: base_time  单位是毫秒，主要用于填充历史视频数据时修正tx_set_video_data接口的nTimeStamps参数，具体用法是，nTimeStamps = frameTimeStamp - base_time，
     *                   frameTimeStamp 精确到毫秒。	 
	 */
	void (*on_play_history_video)(unsigned int play_time, unsigned long long base_time);
} tx_history_video_notify;

/**
 * 使用历史视频功能需要在设备上线之后进行初始化，传入回调方法
 * param: notify 设备回调通知
 */
SDK_API void tx_init_history_video_notify(tx_history_video_notify *notify);

CXX_EXTERN_END

#endif // __IPCAMERA_H__
