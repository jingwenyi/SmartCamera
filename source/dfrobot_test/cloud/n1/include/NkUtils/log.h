/**
 * 日志信息控制单体模块。
 */

#include <NkUtils/types.h>

#ifndef NK_LOG_H_
#define NK_LOG_H_
NK_CPP_EXTERN_BEGIN

/**
 * 日志处理等级。
 */
typedef enum Nk_LogLevel
{
	NK_LOG_ALL = (0),
	NK_LOG_LV_DEBUG,
	NK_LOG_LV_INFO,
	NK_LOG_LV_WARN,
	NK_LOG_LV_ERROR,
	NK_LOG_LV_ALERT,
	NK_LOG_NONE,

} NK_LogLevel;

/**
 * Log 单体模块句柄。
 */
struct Nk_Log
{
	/**
	 * 设置日志保存等级。\n
	 * 设置等级后，要求满足输出等级小于等于设置等级才会进行保存，否则一律丢弃。\n
	 * 日志保存路径由接口 @ref setLogPath 进行设置。\n
	 * 如设置等级为 @ref NK_LOG_LV_NOTICE，则通过接口 @ref error 和接口 @ref notice 的日志模块才会处理，\n
	 * 而接口 @ref info 和接口 @ref debug 的日志模块会自动丢弃。\n
	 * 不设置的情况下，默认是 @ref NK_LOG_LV_ERROR 等级。
	 *
	 */
	NK_Void
	(*setLogLevel)(NK_LogLevel level);

	/**
	 * 获取日志保存等级。
	 */
	NK_LogLevel
	(*getLogLevel)();

	/**
	 * 设置日志控制台输出等级。\n
	 * 只有满足输出等级小于等于设置等级才会在控制台输出，否则忽略。\n
	 * 不设置的情况下，默认是 @ref NK_LOG_LV_INFO 等级。
	 *
	 */
	NK_Void
	(*setTerminalLevel)(NK_LogLevel level);

	/**
	 * 获取日志控制台输出等级。
	 */
	NK_LogLevel
	(*getTerminalLevel)();

	/**
	 * 境界日志记录。
	 */
	NK_Void
	(*alert)(const NK_PChar fmt, ...);

	/**
	 * 错误日志记录。
	 */
	NK_Void
	(*error)(const NK_PChar fmt, ...);

	/**
	 * 通知日志记录。
	 */
	NK_Void
	(*warn)(const NK_PChar fmt, ...);

	/**
	 * 信息日志记录。
	 */
	NK_Void
	(*info)(const NK_PChar fmt, ...);

	/**
	 * 调试信息记录。
	 */
	NK_Void
	(*debug)(const NK_PChar fmt, ...);

	/**
	 * 模块内会保留一块日志缓冲，所有等级小于 Terminal Level 的日志将会被记录到缓冲以内，\n
	 * 当缓冲满时会自动触发此接口擦除，同时会触发 @ref onFlush 事件，\n
	 * 用户可以根据具体实现方案处理在 @ref onFlush 事件中处理缓冲的日志数据。
	 */
	NK_Void
	(*flush)();

	/**
	 * 日志缓冲满冲刷事件。\n
	 * 参考 @ref flush。\n
	 * 须要注意的是，事件实现中不能调用 NK_Log 模块相关接口，否则由可能由于程序死循环造成的崩溃。
	 */
	NK_Void
	(*onFlush)(NK_PByte bytes, NK_Size len);

};

/**
 * 获取单体句柄。
 */
extern struct Nk_Log
*NK_Log();


/**
 * 终端输出表格句柄。
 * 存放表格输出过程中的上下文。
 */
typedef struct Nk_TermTable
{
	NK_Byte reserved[128];

} NK_TermTable;

/**
 * 开始绘制终端表格。
 * 初始化 @ref Tbl 句柄。
 */
extern NK_Int
NK_TermTbl_BeginDraw(NK_TermTable *Tbl, NK_PChar title, NK_Size width, NK_Size padding);

/**
 * 终端表格追加一行明文文本。
 *
 * @param[in]			end_ln			行结束标识，为 True 时明文追加后会加上底部边框。
 * @param[in]			fmt				文本输出格式。
 *
 * @retval 0		输出成功。
 * @retval -1		输出失败。
 *
 */
extern NK_Int
NK_TermTbl_PutText(NK_TermTable *Tbl, NK_Boolean end_ln, NK_PChar fmt, ...);

/**
 * 终端表格追加一行“键-值”格式的文本。。
 *
 * @param[in]			end_ln			行结束标识，为 True 时明文追加后会加上底部边框。
 * @param[in]			key				键文本。
 * @param[in]			fmt				值文本输出格式。
 *
 * @retval 0		输出成功。
 * @retval -1		输出失败。
 *
 */
extern NK_Int
NK_TermTbl_PutKeyValue(NK_TermTable *Tbl, NK_Boolean end_ln, NK_PChar key, NK_PChar fmt, ...);

/**
 * 结束绘制。\n
 * 调用此接口会根据之前的输出情况追加一个底部边框，并复位句柄。
 *
 */
extern NK_Int
NK_TermTbl_EndDraw(NK_TermTable *Tbl);



NK_CPP_EXTERN_END
#endif /* NK_LOG_H_ */
