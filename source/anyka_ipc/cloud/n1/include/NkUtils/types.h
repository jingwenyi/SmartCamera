
/**
 * 基础类型定义。
 *
 * Author: Frank Law
 */

#include <stdlib.h>
#include <stdint.h>

#if defined(__cplusplus)
#  define NK_CPP_EXTERN_BEGIN			extern "C" {
#  define NK_CPP_EXTERN_END			}
#else
#  define NK_CPP_EXTERN_BEGIN
#  define NK_CPP_EXTERN_END
#endif

#if !defined(NK_UTILS_TYPES_H_)
#define NK_UTILS_TYPES_H_
NK_CPP_EXTERN_BEGIN


typedef		int8_t		NK_Int8;
#define		NK_INT8		NK_Int8

typedef		int16_t		NK_Int16;
#define		NK_INT16	NK_Int16

typedef		int32_t		NK_Int32;
#define		NK_INT32	NK_Int32

typedef		int64_t		NK_Int64;
#define		NK_INT64	NK_Int64

typedef		uint8_t		NK_UInt8;
#define		NK_UINT8	NK_UInt8

typedef		uint16_t	NK_UInt16;
#define		NK_UINT16	NK_UInt16

typedef		uint32_t	NK_UInt32;
#define		NK_UINT32	NK_UInt32

typedef		uint64_t	NK_UInt64;
#define		NK_UINT64	NK_UInt64

typedef		char		NK_Char;
#define		NK_CHAR		NK_Char

typedef		NK_Char*	NK_PChar;
#define		NK_PCHAR	NK_PChar

typedef		NK_UInt8	NK_Byte;
#define		NK_BYTE		NK_Byte

typedef		NK_Byte*	NK_PByte;
#define		NK_PBYTE	NK_PByte

typedef		int			NK_Integer;
#define		NK_INTEGER	NK_Integer

typedef		int			NK_Int;
#define		NK_INT		NK_Int

typedef		NK_Int32	NK_SSize;
#define		NK_SSIZE	NK_SSize

typedef		NK_UInt32	NK_Size;
#define		NK_SIZE		NK_Size

typedef		NK_Int64	NK_SSize64;
#define		NK_SSIZE64	NK_SSize64

typedef		NK_UInt64	NK_Size64;
#define		NK_SIZE64	NK_Size64

typedef		void		NK_Void;
#define		NK_VOID		NK_Void

typedef		NK_Void*	NK_PVoid;
#define		NK_PVOID	NK_PVoid

typedef		NK_Integer	NK_Boolean;
#define		NK_BOOLEAN	NK_Boolean

#define		NK_False	(0)
#define 	NK_True		(!NK_False)
#define		NK_FALSE	(NK_False)
#define		NK_TRUE		(NK_True)

#define		NK_Nil		(NULL)//(NK_PVoid)(0)
#define		NK_NIL		(NK_Nil)

typedef		float		NK_Float;
#define		NK_FLOAT	NK_Float

typedef		double		NK_DFloat;
#define		NK_DFLOAT	NK_DFloat

#define inline __inline

/**
 * 世界时间码，此类新变量表示自公元 1970 年 1 月 1 日 GMT 时区经过的秒数。
 */
#define NK_UTC1970		NK_UInt32

/**
 * 时区秒数偏移计数。
 */
#define NK_TZ_GMT_OFF(__hour, __min) ((__hour) * 3600 + (__min) * 60)

/**
 * 时区定义。\n
 * 值为相对格林尼治 0 时区的偏移值。\n
 * 通过与 @ref NK_UTC1970 变量相加可以获得当前时区的时间码。
 */
typedef enum nkTimeZone
{
	NK_TZ_GMT_W1200	= -NK_TZ_GMT_OFF(12, 0),
	NK_TZ_GMT_W1100	= -NK_TZ_GMT_OFF(11, 0),
	NK_TZ_GMT_W1000	= -NK_TZ_GMT_OFF(10, 0),
	NK_TZ_GMT_W0900	= -NK_TZ_GMT_OFF(9, 0),
	NK_TZ_GMT_W0800	= -NK_TZ_GMT_OFF(8, 0),
	NK_TZ_GMT_W0700	= -NK_TZ_GMT_OFF(7, 0),
	NK_TZ_GMT_W0600	= -NK_TZ_GMT_OFF(6, 0),
	NK_TZ_GMT_W0500	= -NK_TZ_GMT_OFF(5, 0),
	NK_TZ_GMT_W0430	= -NK_TZ_GMT_OFF(4, 30),
	NK_TZ_GMT_W0400	= -NK_TZ_GMT_OFF(4, 0),
	NK_TZ_GMT_W0330	= -NK_TZ_GMT_OFF(3, 30),
	NK_TZ_GMT_W0300	= -NK_TZ_GMT_OFF(3, 0),
	NK_TZ_GMT_W0200	= -NK_TZ_GMT_OFF(2, 0),
	NK_TZ_GMT_W0100	= -NK_TZ_GMT_OFF(1, 0),
	NK_TZ_GMT		= NK_TZ_GMT_OFF(0, 0),
	NK_TZ_GMT_E0100	= NK_TZ_GMT_OFF(1, 0),
	NK_TZ_GMT_E0200	= NK_TZ_GMT_OFF(2, 0),
	NK_TZ_GMT_E0300	= NK_TZ_GMT_OFF(3, 0),
	NK_TZ_GMT_E0330	= NK_TZ_GMT_OFF(3, 30),
	NK_TZ_GMT_E0400	= NK_TZ_GMT_OFF(4, 0),
	NK_TZ_GMT_E0430	= NK_TZ_GMT_OFF(4, 30),
	NK_TZ_GMT_E0500	= NK_TZ_GMT_OFF(5, 0),
	NK_TZ_GMT_E0530	= NK_TZ_GMT_OFF(5, 30),
	NK_TZ_GMT_E0545	= NK_TZ_GMT_OFF(5, 45),
	NK_TZ_GMT_E0600	= NK_TZ_GMT_OFF(6, 0),
	NK_TZ_GMT_E0630	= NK_TZ_GMT_OFF(6, 30),
	NK_TZ_GMT_E0700	= NK_TZ_GMT_OFF(7, 0),
	NK_TZ_GMT_E0800	= NK_TZ_GMT_OFF(8, 0),
	NK_TZ_GMT_E0900	= NK_TZ_GMT_OFF(9, 0),
	NK_TZ_GMT_E0930	= NK_TZ_GMT_OFF(9, 30),
	NK_TZ_GMT_E1000	= NK_TZ_GMT_OFF(10, 0),
	NK_TZ_GMT_E1100	= NK_TZ_GMT_OFF(11, 0),
	NK_TZ_GMT_E1200	= NK_TZ_GMT_OFF(12, 0),
	NK_TZ_GMT_E1300	= NK_TZ_GMT_OFF(13, 0),

} NK_TimeZone;


/**
 * 文本编码类型定义。
 */
typedef enum Nk_TextEncoding
{
	NK_TEXT_ENC_UNDEF			= (-1),		///< 未定义。
	NK_TEXT_ENC_ASCII,						///< ASCII 码。
	NK_TEXT_ENC_GB2312,						///< GB2312 简体汉字码。
	NK_TEXT_ENC_GBK,						///< GBK 简体汉字码。
	NK_TEXT_ENC_UTF8,						///< UTF-8 编码。

} NK_TextEncoding;


NK_CPP_EXTERN_END
#endif /* NK_UTILS_TYPES_H_ */
