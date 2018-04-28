/** @file
* @brief Define the ISP config file api
*
* Copyright (C) 2015 Anyka (GuangZhou) Software Technology Co., Ltd.
* @author
* @date 2015-07-24
* @version 1.0
*/

#ifndef __ISP_CONF_FILE_H__
#define __ISP_CONF_FILE_H__

#include "anyka_types.h"

#define ISP_CFG_MAX_SIZE		(1024*32)


T_BOOL ISP_Conf_FileLoad(T_BOOL bDayFlag, T_U8* buf, T_U32* size);

T_BOOL ISP_Conf_FileStore(T_BOOL bDayFlag, T_U8* buf, T_U32 size);


#endif

