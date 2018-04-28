
/**
 * N1 设备 API 接口。
 */

#include <NkUtils/n1_def.h>

#ifndef NK_N1_DEVICE_H_
#define NK_N1_DEVICE_H_
NK_CPP_EXTERN_BEGIN




/**
 * @brief N1 设备模块运行时上下文。
 */
typedef struct Nk_N1Device
{
	/**
	 * 当前设备的序列号。\n
	 * 用户在初始化的时候传入，作为此设备在局域网中连接的唯一性。\n
	 * 若不设置此值，则内部会随机产生一个字符串作为当前设备的唯一标识。
	 */
	NK_Char device_id[128];

	/**
	 * 云设备序列号。\n
	 * 该序列号是设备在互联网通讯上唯一身份码，在生产时统一分配并记录在设备当中。
	 */
	NK_Char cloud_id[32];

	/**
	 * 模块用于监听网络事件的端口。\n
	 * 由于模块使用网络连接方式为 TCP，因此配置此端口时注意不能与模块以外的其他 TCP 绑定端口冲突。\n
	 * 建议使用 1024 以上的端口。
	 */
	NK_UInt16 port;

	/**
	 * 用户上下文。\n
	 * 用于模块触发事件时与用户外部共享数据。
	 */
	NK_PVoid user_ctx;


	struct {

		/**
		 * 流媒体现场抓图事件。\n
		 * 客户端需要预览摄像机现场图片的时候会出发此接口向设备获取一张现场缩略图。\n
		 * 缩略图只支持 JPEG 文件格式。
		 *
		 * @param[in]		channel_id		抓图通道，从 0 开始。
		 * @param[in]		width			图片宽度。
		 * @param[in]		height			图片高度。
		 * @param[in]		pic				图片缓冲。
		 * @param[in,out]	size			缓冲/图片大小。
		 *
		 * @retval NK_N1_ERR_NONE					现场抓图成功。
		 * @retval NK_N1_ERR_DEVICE_BUSY			设备忙碌，客户端有可能会定时重试。
		 * @retval NK_N1_ERR_DEVICE_NOT_SUPPORT		设备不支持此抓图功能，客户段将不再对该设备进行抓图。
		 *
		 */
		NK_N1Error
		(*onLiveSnapshot)(NK_Int channel_id, NK_Size width, NK_Size height, NK_PByte pic, NK_Size *size);


		/**
		 * 流媒体直播连接事件。\n
		 * 客户端连接时会触发此事件。\n
		 * 实现时根据传入的 Session::channel_id 和 Session::stream_id 对应设备情况，\n
		 * 初始化会话 @ref Session 数据结构，\n
		 * 并根据具体实现情况返回相对的数值引导模块做出响应的响应。\n
		 * 其中 @ref Session::user_session 用于保留用户会话，\n
		 * 实现时可以保留实施媒体的数据源相关信息。\n
		 *
		 *
		 * @param[in,out]	Session			会话上下文，模块通过会话上下文与实现共享数据。
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 *
		 * @retval NK_N1_ERR_NONE					连接成功，客户端继续获取数据媒体数据。
		 * @retval NK_N1_ERR_DEVICE_BUSY			设备忙碌，当用户媒体源资源请求失败时返回此值。
		 * @retval NK_N1_ERR_DEVICE_NOT_SUPPORT		设备不支持此连接请求，当客户端请求的通道和码流超出设备提供的范围返回此致。
		 * @retval NK_N1_ERR_NOT_AUTHORIZATED		用户校验失败，请求客户端对应用户不具备此媒体直播权限。
		 *
		 */
		NK_N1Error
		(*onLiveConnected)(NK_N1LiveSession *Session, NK_PVoid ctx);

		/**
		 * 流媒体直播断开事件。\n
		 * 客户端断开连接时模块会触发此事件。\n
		 *
		 * @param[in,out]	Session			会话上下文，模块通过会话上下文与实现共享数据。
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 *
		 * @retval NK_N1_ERR_NONE					断开成功，客户端不再获取数据媒体数据。
		 *
		 */
		NK_N1Error
		(*onLiveDisconnected)(NK_N1LiveSession *Session, NK_PVoid ctx);

		/**
		 * 流媒体直播读取事件。\n
		 * 当流媒体被点播时，客户端获取媒体帧数据会触发该事件。
		 *
		 * @param[in,out]	Session			会话上下文，模块通过会话上下文与实现共享数据。
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 * @param[out]		payload_type	读取的数据载体类型。
		 * @param[out]		ts_ms			读取的数据时间戳（单位：毫秒）。
		 * @param[out]		data_r			读取的数据所在内存的地址。
		 *
		 * @retval 大于0								读取成功，读取的数据长度。
		 * @retval 等于0								读取失败，没有数据可读。
		 * @retval 小于0								读取失败，读取时发生错误，需要退出本会话。
		 *
		 */
		NK_SSize
		(*onLiveReadFrame)(NK_N1LiveSession *Session, NK_PVoid ctx,
				NK_N1DataPayload *payload_type, NK_UInt32 *ts_ms, NK_PByte *data_r);

		/**
		 * 流媒体直播读取结束事件。\n
		 * 在触发事件 @ref onLiveReadFrame() 后库内部会引用数据的内存地址，\n
		 * 当使用完这部分数据以后会触发此事件，便于用户释放引用的数据资源。
		 *
		 *
		 * @param[in,out]	Session			会话上下文，模块通过会话上下文与实现共享数据。
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 * @param[in]		data_r			触发事件 @ref onLiveReadFrame() 时引用的数据内存地址。
		 * @param[in]		size			触发事件 @ref onLiveReadFrame() 时引用的数据大小。
		 *
		 * @retval NK_N1_ERR_NONE					操作成功。
		 */
		NK_N1Error
		(*onLiveAfterReadFrame)(NK_N1LiveSession *Session, NK_PVoid ctx,
				NK_PByte *data_r, NK_Size size);


		/**
		 * 局域网配置事件。
		 *
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 * @param[in]		set_or_get		设置/获取标识，当此值为 True 时候 Setup 为传入参数，反之为传出参数。
		 * @param[in,out]	Setup			设置数据结构。
		 *
		 * @retval NK_N1_ERR_NONE					配置或操作成功。
		 * @retval NK_N1_ERR_DEVICE_NOT_SUPPORT		设备不支持该配置或操作。
		 * @retval NK_N1_ERR_INVALID_PARAM			配置或操作传入了无效的参数。
		 * @retval NK_N1_ERR_NOT_AUTHORIZATED		用户校验失败，请求客户端对应用户不具备此设置权限。
		 */
		NK_N1Error
		(*onLanSetup)(NK_PVoid ctx, NK_Boolean set_or_get, NK_N1LanSetup *Setup);


		/**
		 * 扫描无线热点事件。\n
		 * 当模块须要检测附近的 WiFi NVR 的时候触发此事件。\n
		 * 用户通过实现此接口，告知模块附近的 WiFi 热点信息。
		 *
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 * @param[out]		HotSpots		热点数据结构栈区缓冲。
		 * @param[in,out]	n_hotSpots		传入热点数据结构栈区的缓冲，传出实际扫描到热点的数量。
		 *
		 * @retval NK_N1_ERR_NONE					扫描热点成功，结果保存在 @ref HotSpots 数组中。
		 *
		 */
		NK_N1Error
		(*onScanWiFiHotSpot)(NK_PVoid ctx, NK_WiFiHotSpot *HotSpots, NK_Size *n_hotSpots);


		/**
		 * 连接无线热点事件。\n
		 * 对于支持无线连接的设备，当模块希望用户连接某个 NVR 设备的时候会触发此事件，\n
		 * 触发此事件时，同样会发起一个配置 @ref NK_N1LanSetup::WiFiNVR 的事件，\n
		 * 与 @ref onLanSetup 事件不同的时候，这里发起配置 @ref NK_N1LanSetup::WiFiNVR 的事件时，\n
		 * 用户不需要保存 @ref NK_N1LanSetup::WiFiNVR 的配置数据，\n
		 * 而 @ref onLanSetup 事件则须要用户保存 @ref NK_N1LanSetup::WiFiNVR 对应数据结构。\n
		 *
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 * @param[in,out]	Setup			局域网配置数据结构。
		 *
		 * @retval NK_N1_ERR_NONE					连接热点成功。
		 */
		NK_N1Error
		(*onConnectWiFiHotSpot)(NK_PVoid ctx, NK_N1LanSetup *Setup);


		/**
		 * 用户更新事件。\n
		 * 当校验用户状态更新时会触发此事件，\n
		 * 实现此接口保存最新的用户数据。
		 *
		 * @param[in,out]	ctx				用户上下文，在调用接口 @ref NK_N1Device_Init 时传入。
		 *
		 * @retval NK_N1_ERR_NONE				操作成功。
		 */
		NK_N1Error
		(*onUserChanged)(NK_PVoid ctx);


	} EventSet;


} NK_N1Device;


/**
 * 获取 N1 版本号。\n
 * 用于判断二进制库文件与头文件是否匹配，避免由于数据结构大小不匹配而产生潜在的问题。
 *
 * @param[out]			ver_maj				与 NK_N1_VER_MAJ 对应。
 * @param[out]			ver_min				与 NK_N1_VER_MIN 对应。
 * @param[out]			ver_rev				与 NK_N1_VER_REV 对应。
 * @param[out]			ver_num				与 NK_N1_VER_NUM 对应。
 *
 */
extern NK_Void
NK_N1Device_Version(NK_UInt32 *ver_maj, NK_UInt32 *ver_min, NK_UInt32 *ver_rev, NK_UInt32 *ver_num);


/**
 * 初始化 N1 设备运行环境。
 *
 * @param[in]			Device				设备初始化句柄，由用户填充后传入。
 *
 * @return		成功返回 0，失败返回 -1。
 */
extern NK_Int
NK_N1Device_Init(NK_N1Device *Device);

/**
 * 销毁 N1 设备运行环境。\n
 * 调用此接口前需要先调用 @ref NK_N1Device_Init 接口此接口才能成功返回。\n
 * 当接口成功返回 0 时，可以通过 @ref user_ctx_r 获取 @ref NK_N1Device_Init 传入的用户上下文。\n
 * 调用者可一个根据设计需求释放传入的用户上下文相关资源。
 *
 * @param[out]			Device				返回初始化时 @ref NK_N1Device_Init 传入的用户上下文。
 *
 * @return		成功返回 0，失败返回 -1。
 */
extern NK_Int
NK_N1Device_Destroy(NK_N1Device *Device);


/**
 * 向客户端发送通知信息。
 *
 */
extern NK_Int
NK_N1Device_Notify(NK_N1Notification *Notif);


/**
 * WiFi NVR 配对控制。\n
 * 如果设备具备 Wi-Fi 连接的能力，可以通过调用此接口使设备域 Wi-Fi NVR 进行配对连接。\n
 * 参数 @ref enabled 决定此方式时开启 Wi-Fi NVR 配对还是关闭。\n
 * 当开启 Wi-Fi NVR 配对时，\n
 * 会在后台开启一个线程，维持与 Wi-Fi NVR 无线连接，直至关闭配对位置。\n
 * 在与 Wi-Fi NVR 维持无线连接的时候，\n
 * 会触发 NK_N1Device::EventSet::onScanHotSpots 和 NK_N1Device::EventSet::onConnectWiFiHotSpot 事件，\n
 * 用户须对以上接口实现才能使 Wi-Fi NVR 配对功能正常工作。
 *
 *
 * @param[in]			enabled				开启 / 关闭 Wi-Fi NVR 配对标识。
 *
 * @return		成功返回 0，失败返回 -1。
 */
extern NK_Int
NK_N1Device_PairWiFiNVR(NK_Boolean enabled);


/**
 * 加入一个认证用户，\n
 * 若不加入任何的用户，协议中所有网络通讯则不进行校验。
 *
 * @param[in]			username			用户的用户名称。
 * @param[in]			password			用户的校验密码。
 * @param[in]			forbidden			用户禁止权限，位有效，目前不使用。
 *
 * @return		成功返回 0，失败返回 -1。
 */
extern NK_Int
NK_N1Device_AddUser(NK_PChar username, NK_PChar password, NK_UInt32 forbidden);


/**
 * 删除一个认证用户。
 *
 * @param[in]			username			用户的用户名称。
 *
 * @return		成功返回 0，失败返回 -1。
 */
extern NK_Int
NK_N1Device_DeleteUser(NK_PChar username);

/**
 * 获取对应用户名称的信息，\n
 * 如果用户存在则发挥该用户的用户密码 @ref password 和禁止权限标识 @ref forbiden_r。
 *
 * @param[in]			username			用户的用户名称。
 * @param[out]			password			用户的校验密码。
 * @param[out]			forbidden_r			用户禁止权限标识。
 *
 * @return		用户存在返回 True，并从 @ref password 和 @ref forbidden_r 获取相关信息，用户不存在则返回 False。
 */
extern NK_Boolean
NK_N1Device_HasUser(NK_PChar username, NK_PChar password, NK_UInt32 *forbidden_r);


/**
 * 获取认证的用户数。
 *
 * @return		用户数量，如果用户数量为 0，则网络通讯不作任何认证。
 */
extern NK_Size
NK_N1Device_CountUser();

/**
 * 获取序列号对应的用户信息，\n
 * 一般用于遍历操作。
 *
 * @param[in]			id					用户序号。
 * @param[out]			username			用户的用户名称。
 * @param[out]			password			用户的校验密码。
 * @param[out]			forbidden_r			用户禁止权限标识。
 *
 * @return		用户存在则返回 0，并且从参数中返回用户信息，失败返回 -1。
 */
extern NK_Int
NK_N1Device_IndexOfUser(NK_Int id, NK_PChar username, NK_PChar password, NK_UInt32 *forbidden_r);


NK_CPP_EXTERN_END
#endif /* NK_N1_DEVICE_H_ */
