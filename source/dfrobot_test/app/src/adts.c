/*
** filename :adts.c
** author: jingwenyi
** date: 2016.06.17
*/

#include "adts.h"
#include <assert.h>


static unsigned const samplingFrequencyTable[16] = {
  96000, 88200, 64000, 48000,
  44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000,
  7350, 0, 0, 0
};


typedef struct PutBitContext {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
    int size_in_bits;
} PutBitContext;


/**
 * Initialize the PutBitContext s.
 *
 * @param buffer the buffer where to put bits
 * @param buffer_size the size in bytes of buffer
 */
static inline void init_put_bits(PutBitContext *s, uint8_t *buffer, int buffer_size)
{
    if(buffer_size < 0) {
        buffer_size = 0;
        buffer = NULL;
    }

    s->size_in_bits= 8*buffer_size;
    s->buf = buffer;
    s->buf_end = s->buf + buffer_size;

    s->buf_ptr = s->buf;
    s->bit_left=32;
    s->bit_buf=0;
}


/**
 * Pad the end of the output stream with zeros.
 */
static inline void flush_put_bits(PutBitContext *s)
{

    s->bit_buf<<= s->bit_left;

    while (s->bit_left < 32) {
        /* XXX: should test end of buffer */
        *s->buf_ptr++=s->bit_buf >> 24;
        s->bit_buf<<=8;
        s->bit_left+=8;
    }
    s->bit_left=32;
    s->bit_buf=0;
}


static inline void put_bits(PutBitContext *s, int n, unsigned int value)
{
	unsigned int bit_buf;
	int bit_left;
	assert(n <= 31 && value < (1U << n));
	
	bit_buf = s->bit_buf;
    bit_left = s->bit_left;
	
	if(n < bit_left)
	{
		bit_buf = (bit_buf << n) | value;
		bit_left -= n;
	}
	else
	{
		bit_buf <<= bit_left;
		bit_buf |= value >> (n - bit_left);
		
		//*(uint32_t*)s->buf_ptr = bit_buf;
		int i=4;
		while(i)
		{
			s->buf_ptr[i-1] = (bit_buf >> (4-i)*8) & 0xff;
			i--;
		}
		
		s->buf_ptr += 4;
		bit_left += 32 - n;
		bit_buf = value;
	}

	
	s->bit_buf = bit_buf;
    s->bit_left = bit_left;
}




void setADTSContext(ADTSContext * ctx, int profile, int sample_rate, int channel)
{
	ctx->objecttype = profile;// <3
	ctx->channel_conf = channel; // 1 or 2

	
	ctx->sample_rate_index = 15;
	int i=0;
	for(i=0; i<13; i++)
	{
		if(sample_rate == samplingFrequencyTable[i])
		{
			ctx->sample_rate_index = i;
			break;
		}
	}
}





int adts_write_frame_header(ADTSContext * ctx, uint8_t *buf, int size, int pce_size)//pce_size = 0
{

	PutBitContext pb;

    init_put_bits(&pb, buf, ADTS_HEADER_SIZE);

    /* adts_fixed_header */
    put_bits(&pb, 12, 0xfff);   /* syncword */
    put_bits(&pb, 1, 0);        /* ID */
    put_bits(&pb, 2, 0);        /* layer */
    put_bits(&pb, 1, 1);        /* protection_absent */
    put_bits(&pb, 2, ctx->objecttype); /* profile_objecttype */
    put_bits(&pb, 4, ctx->sample_rate_index);
    put_bits(&pb, 1, 0);        /* private_bit */
    put_bits(&pb, 3, ctx->channel_conf); /* channel_configuration */
    put_bits(&pb, 1, 0);        /* original_copy */
    put_bits(&pb, 1, 0);        /* home */


    /* adts_variable_header */
    put_bits(&pb, 1, 0);        /* copyright_identification_bit */
    put_bits(&pb, 1, 0);        /* copyright_identification_start */
    put_bits(&pb, 13, ADTS_HEADER_SIZE + size + pce_size); /* aac_frame_length */
    put_bits(&pb, 11, 0x7ff);   /* adts_buffer_fullness */
    put_bits(&pb, 2, 0);        /* number_of_raw_data_blocks_in_frame */
	
    flush_put_bits(&pb);

    return 0;
}









