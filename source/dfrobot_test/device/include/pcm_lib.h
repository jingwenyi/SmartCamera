#ifndef __PCM_LIB_H__
#define __PCM_LIB_H__ 

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pcm_pars {
	unsigned int samplerate;
	unsigned int samplebits;
} pcm_pars_t;

typedef enum
{
	PCM_DEV_DA,
	PCM_DEV_AD,
} pcm_dev_t;

typedef enum
{
	SET_RSTBUF,

	/* DA commands */
	DA_SET_PARS,
	DA_SET_DEV_HP,
	DA_SET_DEV_LO,
	DA_SET_GAIN,
	DA_SET_SDF,

	/* AD commands */
	AD_SET_PARS,
	AD_SET_DEV_MIC,
	AD_SET_DEV_LI,
	AD_SET_GAIN,
	AD_SET_SDF,
} pcm_cmd_t;

/*
 * @brief	open the AD/DA device
 * @author
 * @date
 * @param[in]	dev					AD or DA device.
 * @return void *
 * @retval if return NULL failed, otherwise success
 */
void *pcm_open(pcm_dev_t dev);

/*
 * @brief	control the AD/DA device
 * @author
 * @date
 * @param[in]	fd					AD or DA device handle.
 * @param[in]	cmd					command.
 * @param[in]	value				parameter.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_ioctl(void *fd, pcm_cmd_t cmd, void *value);

/*
 * @brief	read from the AD/DA device
 * @author
 * @date
 * @param[in]	fd					AD or DA device handle.
 * @param[in]	to					read data to the buffer.
 * @param[in]	size				read length.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_read(void *fd, unsigned char *to, unsigned int size);

/*
 * @brief	write to the AD/DA device
 * @author
 * @date
 * @param[in]	fd					AD or DA device handle.
 * @param[in]	from				write data from the buffer.
 * @param[in]	size				write length.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_write(void *fd, unsigned char *from, unsigned int size);

/*
 * @brief	close the AD/DA device
 * @author
 * @date
 * @param[in]	fd 					AD or DA device handle.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_close(void *fd);

/*
 * @brief	AEC switch
 * @author
 * @date
 * @param[in]	enable 				0:disable AEC, 1:enable AEC.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_set_aec_enable(int enable);

/*
 * @brief	dac/adc nr&agc switch
 * @author
 * @date
 * @param[in]	dev					ADC or DAC dev.
 * @param[in]	enable 				0:disable nr&agc, others:enable nr&agc.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_set_nr_agc_enable(pcm_dev_t dev, int enable);

/*
 * @brief	 enable/disable speaker
 * @author
 * @date
 * @param[in]	enable 				0:disable speaker, others:enable speaker.
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_set_speaker_enable(int enable);

/*
 * @brief	get adc/dac time stamp, unit microsecond
 * @author
 * @date
 * @param[out]	timestamp_ms		timestamp unit microsecond
 * @return int
 * @retval if return 0 success, otherwise failed
 */
int pcm_get_timer(void *fd, unsigned long *timestamp_ms);

/*
 * @brief	get adc/dac working status
 * @author
 * @date
 * @param
 * @return int
 * @retval if return 0 don't working, otherwise working.
 */
int pcm_get_status(pcm_dev_t dev);

/*
 * @brief	get adc/dac actually samperate
 * @author
 * @date
 * @param handle after pcm_open call
 * @return unsigned int
 * @retval the actually sr
 */
unsigned int pcm_get_act_sr(void *fd);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
