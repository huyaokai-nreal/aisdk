/*****************************************************************************
Copyright: 2021-2025, Artosyn. Co., Ltd.
File name: ar_hal_type.h
Description: The common data type defination.
Author: Artosyn Software Team
Version: 0.0.1
Date: 2021/04/23
History:
        0.0.1    2021/04/23    The initial version of ar_hal_type.h
*****************************************************************************/

#ifndef __HAL_TYPE_H__
#define __HAL_TYPE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

/*----------------------------------------------*
 * The common data type, will be used in the whole project.*
 *----------------------------------------------*/

typedef unsigned char           AR_U8;
typedef unsigned short          AR_U16;
typedef unsigned int            AR_U32;
typedef signed char             AR_S8;
typedef short                   AR_S16;
typedef int                     AR_S32;
typedef float                   AR_FLOAT;
typedef double                  AR_DOUBLE;
typedef unsigned long long      AR_U64;
typedef long long               AR_S64;
typedef char                    AR_CHAR;
typedef unsigned char           AR_UCHAR;
typedef unsigned long           AR_HANDLE;

#define AR_VOID                 void

//on 64bit platform

typedef long                    AR_INTPTR;

typedef unsigned long           AR_UINTPTR;

/*----------------------------------------------*
 * const defination                             *
 *----------------------------------------------*/
typedef enum {
    AR_FALSE = 0,
    AR_TRUE  = 1,
} AR_BOOL;


#ifndef NULL
    #define NULL        0L
#endif

#define AR_NULL         0L
#define AR_SUCCESS      0
#define AR_FAILURE      (-1)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_TYPE_H__ */

