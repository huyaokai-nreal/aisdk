#ifndef __AR_SYS_DRV_H__
#define __AR_SYS_DRV_H__

#include "osal_ioctl.h"

#define LOG_TAG_MAX_LEN     16
#define LOG_MOD_MAX_NUM     95
#define LOG_RSV_ID_START    75
#define LOG_RSV_NUM         LOG_MOD_MAX_NUM - LOG_RSV_ID_START
#define LOG_QUERY_NUM       3*LOG_RSV_ID_START + LOG_RSV_NUM

#define LOG_MOD_ID_MASK     0x0000ffff
#define LOG_LAYER_ID_MASK   0x000f0000

#define LOG_LAYER_CORE      0x00010000
#define LOG_LAYER_HAL       0x00020000
#define LOG_LAYER_MPP       0x00030000

typedef struct {
	char			ch_gps_latitude_ref;		/* GPS LatitudeRef Indicates whether the latitude is north or south latitude,
												 * 'N'/'S', default 'N' */
	unsigned int	au32_gps_latitude[3][2];	/* GPS Latitude is expressed as degrees, minutes and seconds, a typical format
												 * like "dd/1, mm/1, ss/1", default 0/0, 0/0, 0/0 */
	char			ch_gps_longitude_ref;		/* GPS LongitudeRef Indicates whether the longitude is east or west longitude,
												 * 'E'/'W', default 'E' */
	unsigned int	au32_gps_longitude[3][2];	/* GPS Longitude is expressed as degrees, minutes and seconds, a typical format
												 * like "dd/1, mm/1, ss/1", default 0/0, 0/0, 0/0 */
	unsigned char   u8_gps_altitude_ref;		/* GPS AltitudeRef Indicates the reference altitude used, 0 - above sea level,
												 * 1 - below sea level default 0 */
	unsigned int	au32_gps_altitude[2];		/* GPS AltitudeRef Indicates the altitude based on the reference u8GPSAltitudeRef,
												 * the reference unit is meters, default 0/0 */
} STRU_SYS_GPS_INFO;

typedef struct {
	int  id;
	char tag[LOG_TAG_MAX_LEN];
	int  level;
} STRU_MOD_INFO;

typedef struct {
	STRU_MOD_INFO log_info[LOG_QUERY_NUM];
	int num;
} STRU_QUERY_INFO;

#define SSRAM_CFG_LEN   512

typedef struct {
	char config[SSRAM_CFG_LEN];
} STRU_SRAM_CONFIG_INFO;

//1.register  id & LOG_MOD_ID_MASK  <===>  base_tag;  if id=-1; retrun reseved id;
//2.read ->   in id  out level/all_tag
//3.write->   in id  level
//4.query     if id = -1 ; end

#define IOC_SYS_FLUSH_DCACHE_DIRTY	_IOW('y', 20, struct dirty_area)
#define IOC_SYS_SET_TIME_ZONE		_IOW('y', 23, int)
#define IOC_SYS_GET_TIME_ZONE		_IOR('y', 24, int)
#define IOC_SYS_SET_GPS_INFO		_IOW('y', 25, STRU_SYS_GPS_INFO)
#define IOC_SYS_GET_GPS_INFO		_IOR('y', 26, STRU_SYS_GPS_INFO)

#define IOC_SYS_INIT_PTS_BASE		_IOW('p', 21, unsigned long long)
#define IOC_SYS_GET_CUR_PTS			_IOR('p', 22, unsigned long long)
#define IOC_SYS_SYNC_PTS			_IOW('p', 23, unsigned long long)
#define IOC_SYS_GET_PTS_OFFSET		_IOR('p', 24, unsigned long long)

#define IOC_SYS_GET_LOG_LEVEL		_IOR('p', 25, STRU_MOD_INFO)
#define IOC_SYS_SET_LOG_LEVEL		_IOW('p', 26, STRU_MOD_INFO)
#define IOC_SYS_REGISTER_LOG_TAG	_IOR('p', 27, STRU_MOD_INFO)
#define IOC_SYS_QUERY_LOG_TAG	    _IOR('p', 28, STRU_QUERY_INFO)

#define IOC_SYS_GET_MPP_SERVICE_TYPE    _IOR('p', 29, int)
#define IOC_SYS_GET_SRAM_CONFIG     _IOR('p', 30, STRU_SRAM_CONFIG_INFO)

#endif
