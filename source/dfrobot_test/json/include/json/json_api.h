/*
** author:	wenyi.jing
** date:		2016.05.11
** company: dfrobot
*/

#ifndef _JSON_API_H_
#define _JSON_API_H_

//#include "../colorlut.h"


struct ColorSignature
{
	ColorSignature()
	{
		m_uMin = m_uMax = m_uMean = m_vMin = m_vMax = m_vMean = m_type = 0;
	}	

    int32_t m_uMin;
    int32_t m_uMax;
    int32_t m_uMean;
    int32_t m_vMin;
    int32_t m_vMax;
    int32_t m_vMean;
	uint32_t m_rgb;
	uint32_t m_type;
};


ColorSignature *json_load_file(const char *file_name);

int json_write_file(const ColorSignature *json_signatures_array, const char *file_name);




#endif //_JSON_API_H_

