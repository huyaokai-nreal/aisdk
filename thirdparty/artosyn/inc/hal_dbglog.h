/**
 * @file    hal_dbglog.h
 * @brief   hal调试信息模块数据结构和API
 * @author  Artosyn Software Team
 * @version 0.0.1
 * @date   2021/04/21
 * @license   2021-2025, Artosyn. Co., Ltd.
**/
#ifndef __AR_HAL_DBGLOG_H__
#define __AR_HAL_DBGLOG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


//=============================================================================
// Include files
//=============================================================================
#include "hal_type.h"
#include "hal_comm.h"
#include "utils_dbglog.h"

//=============================================================================
// Macro definition
//=============================================================================
#define LOG_MODULE_MAX_NUM 71

#define HAL_TAG_ID(mod_id) ( mod_id | LOG_LAYER_HAL)

//=============================================================================
// Global function definition
//=============================================================================
/**
* @brief  初始化日志模块
* @param  无
* @retval 0 代表函数执行成功
* @note   使用日志库前必须调用该函数
*/
AR_S32 ar_hal_log_init();

/**
* @brief  关闭日志模块
* @param  无
* @retval 无
* @note   关闭日志库须调用该函数
*/
AR_VOID ar_hal_log_close();

/**
* @brief  设置模块日志等级
* @param  pstConf 为日志等级信息结构体
* @retval 0 代表函数执行成功
* @note   当pstConf->ModNam为""，函数将使用默认模块名；
*		  当pstConf->ModNam为"all"，将设置全部模块的日志等级。
*/
AR_S32 ar_hal_log_set_level_conf(STRU_LOG_LEVEL_CONF *pst_conf);

/**
* @brief  获取模块日志等级
* @param  pstConf 为日志等级信息结构体
* @retval 0 代表函数执行成功
*/
AR_S32 ar_hal_log_get_level_conf(STRU_LOG_LEVEL_CONF *pst_conf);

//TODO
AR_S32 ar_hal_log_set_wait_flag(AR_S32 s32_bWait);


/**
* @brief  读取日志信息
* @param  pBuf 用于存放日志的内存指针
* @param  u32Size 读取日志的大小
* @retval 0 代表函数执行成功
*/
AR_S32 ar_hal_log_read(AR_CHAR *ps8_buf, AR_U32 u32_size);


const AR_CHAR* ar_hal_log_get_module_name(int u32_mode_id);

AR_S32 ar_hal_log_get_name_by_id(int id, char *name);

/**
* used internal
*/
//void ar_hal_log_func(uint8_t level, int tag_id, const char *file, const char *func,
//					const int line, const char *format, ...);

//#define AR_LOG_(tag_id, level, ...)		ar_hal_log_func(level, tag_id, FILENAME(__FILE__), __FUNCTION__, __LINE__, __VA_ARGS__)

/**
* different levels of printing interface
*/
//#define AR_LOG_ASSERT(tag_id, ...)	    AR_LOG_(tag_id, AR_LOG_LEVEL_ASSERT, __VA_ARGS__)
//#define AR_LOG_ERR(tag_id, ...)			AR_LOG_(tag_id, AR_LOG_LEVEL_ERROR, __VA_ARGS__)
//#define AR_LOG_WARN(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_WARN, __VA_ARGS__)
//#define AR_LOG_INFO(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_INFO, __VA_ARGS__)
//#define AR_LOG_DBG(tag_id, ...)			AR_LOG_(tag_id, AR_LOG_LEVEL_DEBUG, __VA_ARGS__)
//#define AR_LOG_VERBOSE(tag_id, ...)		AR_LOG_(tag_id, AR_LOG_LEVEL_VERBOSE, __VA_ARGS__)


#ifdef __cplusplus
#if __cplusplus
	}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /*__AR_HAL_DBGLOG_H__ */
