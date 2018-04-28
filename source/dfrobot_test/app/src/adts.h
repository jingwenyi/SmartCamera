/*
** filename: adts.h
**  author: jingwenyi
**  date: 2016.06.17
*/

#ifndef _ADTS_H_
#define _ADTS_H_

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ADTS_HEADER_SIZE 7
#define MAX_PCE_SIZE 304 ///<Maximum size of a PCE including the 3-bit ID_PCE
                   


typedef struct {
    int write_adts;
    int objecttype;
    int sample_rate_index;
    int channel_conf;
    int pce_size;
    uint8_t pce_data[MAX_PCE_SIZE];
} ADTSContext;


void setADTSContext(ADTSContext * ctx, int profile, int sample_rate, int channel);



int adts_write_frame_header(ADTSContext * ctx, uint8_t *buf, int size, int pce_size);


#ifdef __cplusplus
} /* end extern "C" */
#endif


#endif //_ADTS_H_


