/*****************************************************************************
Copyright: 2021-2025, Artosyn. Co., Ltd.
File name: utils_dbglog.h
Description: The external debug log APIs.
Author: Artosyn Software Team
Version: 0.0.1
Date: 2021/04/21
History:
        0.0.1    2021/04/21    The initial version of ar_hal_dbglog.h
*****************************************************************************/


#ifndef __UTILS_DBGLOG_H__
#define __UTILS_DBGLOG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


//=============================================================================
// Include files
//=============================================================================
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ar_sys_drv.h"

//=============================================================================
// Data type definition
//=============================================================================
/* all formats index */
typedef enum {
    AR_FMT_LVL    = 1 << 0, /**< level */
    AR_FMT_TAG    = 1 << 1, /**< tag */
    AR_FMT_TIME   = 1 << 2, /**< current time */
    AR_FMT_P_INFO = 1 << 3, /**< process info */
    AR_FMT_T_INFO = 1 << 4, /**< thread info */
    AR_FMT_DIR    = 1 << 5, /**< file directory and name */
    AR_FMT_FUNC   = 1 << 6, /**< function name */
    AR_FMT_LINE   = 1 << 7, /**< line number */
} ENUM_LOG_FMT_INDEX;


//=============================================================================
// Macro definition
//=============================================================================
#define FILENAME(x) strrchr(x, '/') ?  strrchr(x, '/') + 1 : x

/* output log's level */
#define AR_LOG_LEVEL_ASSERT                      0
#define AR_LOG_LEVEL_ERROR                       1
#define AR_LOG_LEVEL_WARN                        2
#define AR_LOG_LEVEL_INFO                        3
#define AR_LOG_LEVEL_DEBUG                       4
#define AR_LOG_LEVEL_VERBOSE                     5

/* macro definition for all formats */
#define AR_LOG_FMT_ALL    (AR_FMT_LVL|AR_FMT_TAG|AR_FMT_TIME|AR_FMT_P_INFO|AR_FMT_T_INFO| \
    AR_FMT_DIR|AR_FMT_FUNC|AR_FMT_LINE)

//=============================================================================
// Global function definition
//=============================================================================
/**
* @brief  Initialize the log.
* @param  NONE.
* @retval 0    means the function is well done.
* @note   This function must be called when use log.
*/
int ar_log_init();

/**
* @brief  Close the log.
* @param  NONE.
* @retval NONE.
* @note   This function must be called when close log.
*/
void ar_log_close();

/**
* @brief  Set log output status.
* @param  enabled	the output status of log.
* @retval NONE.
*/
void ar_log_set_output_enabled(bool enabled);

/**
* @brief  Get log output status.
* @param  NONE.
* @retval The status of the log output status.
*/
bool ar_log_get_output_enabled();

/**
* @brief  Set text color status.
* @param  enabled	the text color status of log.
* @retval NONE.
*/
void ar_log_set_textcolor_enabled(bool enabled);

/**
* @brief  Get text color status.
* @param  NONE.
* @retval The status of the text color.
*/
bool ar_log_get_textcolor_enabled();
/**
* @brief  Set log file config.
* @param  base_name    file base name, default:"elog_file.log"
* @param  max_size.    file max size, default: 1*1024*1024
* @param  max_rotate   file max rotate num, default: 5
* @retval The status of the text color..
*/
bool ar_log_set_file_cfg(char       *base_name, size_t max_size, uint32_t max_rotate);

/**
* @brief  Set log file status.
* @param  enable or disable log file.
* @retval NONE.
*/
void ar_log_set_file_enabled(bool enabled);

/**
* @brief  Get log file status.
* @param  NONE.
* @retval The status of the text color..
*/
bool ar_log_get_file_enabled();

/**
* @brief  Set log fmt.
* @param  set	the fmt of log.
* @retval NONE.
*/
void ar_log_set_Fmt(uint32_t set);

void ar_log_shm_set_enabled(bool enabled);

bool ar_log_shm_get_enabled();
int ar_log_shm_read_index_reset(int is_complete_read);

size_t ar_log_shm_get_log(char *log, size_t size);

/**
* used interna
*/
//void ar_hal_log_func(uint8_t level, const char *tag, const char *file, const char *func,
//					const int line, const char *format, ...);

int ar_log_register(STRU_MOD_INFO *log_info);

int ar_read_log_info(STRU_MOD_INFO *log_info);

int ar_write_log_info(STRU_MOD_INFO *log_info);

int ar_query_log_info(STRU_QUERY_INFO *query_info);

/**
* used internal
*/
void ar_log_func(uint8_t level, int tag_id, const char *file, const char *func,
						const int line, const char *format, ...);

void ar_log_func_raw(const char *format, ...);

#define AR_LOG_(tag_id, level, ...)		ar_log_func(level, tag_id, FILENAME(__FILE__), __FUNCTION__, __LINE__, __VA_ARGS__)

/**
* different levels of printing interface
*/
#define AR_LOG_ASSERT(tag_id, ...)	    AR_LOG_(tag_id, AR_LOG_LEVEL_ASSERT, __VA_ARGS__)
#define AR_LOG_ERR(tag_id, ...)			AR_LOG_(tag_id, AR_LOG_LEVEL_ERROR, __VA_ARGS__)
#define AR_LOG_WARN(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_WARN, __VA_ARGS__)
#define AR_LOG_INFO(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_INFO, __VA_ARGS__)
#define AR_LOG_DBG(tag_id, ...)			AR_LOG_(tag_id, AR_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define AR_LOG_VERBOSE(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_VERBOSE, __VA_ARGS__)

#define AR_LOG_RAW(...)                 ar_log_func_raw(__VA_ARGS__)


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /*__AR_HAL_DBGLOG_H__ */

