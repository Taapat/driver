/**********************************文件头部注释************************************/
//
//
//                      Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名：        ywgpio_ext.h
//
// 创建者：        CS
//
// 创建时间：    2008.04.23
//
// 文件描述：    GPIO模块的头文件。
//
// 修改记录：   日       期      作      者       版本      修定
//                       ---------         ---------        -----        -----
//                             2008.04.23         CS                   0.01           新建
//
/*****************************************************************************************/


#ifndef __YWGPIO_EXT_H
#define __YWGPIO_EXT_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct YWLIB_ListHead_s
{
	struct YWLIB_ListHead_s *Next, *Prev;
}YWLIB_ListHead_T;

/*Error Define*/
enum
{
    YWGPIO_ERROR_INIT  = YW_MODULE_SET_ID(YWHAL_MODULE_GPIO_ID),
    YWGPIO_ERROR_TERM,
    YWGPIO_ERROR_OPEN,
    YWGPIO_ERROR_CLOSE,
    YWGPIO_ERROR_READ,
    YWGPIO_ERROR_WRITE,
    YWGPIO_ERROR_CONFIG,
    YWGPIO_ERROR_CTROL,
    YWGPIO_ERROR_BUSY
};

/*Gpio device list name*/
//#define YWGPIO_LIST_NAME  YWGPIO_GpioList

/*Gpio IO Mode*/
typedef enum YWGPIO_IOMode_e
{
    YWGPIO_IO_MODE_NOT_SPECIFIED,
    YWGPIO_IO_MODE_BIDIRECTIONAL,
    YWGPIO_IO_MODE_OUTPUT,
    YWGPIO_IO_MODE_INPUT,
    YWGPIO_IO_MODE_ALTERNATE_OUTPUT,
    YWGPIO_IO_MODE_ALTERNATE_BIDIRECTIONAL,
    YWGPIO_IO_MODE_BIDIRECTIONAL_HIGH,            /*4th bit remains HIGH*/
    YWGPIO_IO_MODE_OUTPUT_HIGH                    /*4th bit remains HIGH*/
}YWGPIO_IOMode_T;

/*Gpio Work Mode*/
typedef enum YWGPIO_WorkMode_e
{
    YWGPIO_WORK_MODE_PRIMARY,   /* Configure for primary functionality */
    YWGPIO_WORK_MODE_SECONDARY /* Configure for GPIO functionality */
} YWGPIO_WorkMode_T;

/*Gpio Pin Interrupt EnmTrigMode*/
typedef enum YWGPIO_InterruptMode_e
{
    YWGPIO_INT_NONE          = 0,
    YWGPIO_INT_HIGH          = 1,
    YWGPIO_INT_LOW           = 2,
    YWGPIO_INT_RAISING_EDGE  = 4,
    YWGPIO_INT_FALLING_EDGE  = 8,
    YWGPIO_INT_VERGE         = 16
} YWGPIO_InterruptMode_T;

/*YWGPIO_Handle_T*/
typedef U32 YWGPIO_Handle_T;

/*YWGPIO_Index_T*/
typedef U32 YWGPIO_Index_T;

/*Gpio Feature List*/
typedef struct YWGPIO_GpioDeviceList_s
{
    YWLIB_ListHead_T     YWGPIO_GpioList;
    U32                          GpioIndex;
    void *                       PrivateData;
} YWGPIO_GpioDeviceList_T;


/*Gpio Feature*/
typedef struct YWGPIO_Feature_s
{
    U32                             GpioNum;        /*Gpio数量*/
    YWLIB_ListHead_T        *GpioListHead;    /*Gpio链表句柄*/
} YWGPIO_Feature_T;

/*Gpio OpenParams*/
typedef struct YWGPIO_OpenParams_s
{
    U32                            GpioIndex;
    YWGPIO_IOMode_T     IoMode;         /*Io模式*/
    YWGPIO_WorkMode_T WorkMode;         /*工作模式*/
    void *  PrivateData;
 }  YWGPIO_OpenParams_T;


typedef void (* YWGPIO_ISRFunc_T)(YWGPIO_Handle_T GpioHandle);

YW_ErrorType_T YWGPIO_Init(void);
YW_ErrorType_T YWGPIO_Term( void);
YW_ErrorType_T YWGPIO_GetFeature( YWGPIO_Feature_T *GpioFeature);
YW_ErrorType_T YWGPIO_Open ( YWGPIO_Handle_T * pGpioHandle,
                                               YWGPIO_OpenParams_T *GpioOpenParams);
YW_ErrorType_T YWGPIO_Close( YWGPIO_Handle_T GpioHandle);
YW_ErrorType_T YWGPIO_SetIoMode ( YWGPIO_Handle_T GpioHandle,
                                                        YWGPIO_IOMode_T IoMode );
YW_ErrorType_T YWGPIO_SetWorkMode ( YWGPIO_Handle_T GpioHandle,
                                                           YWGPIO_WorkMode_T WorkMode );

YW_ErrorType_T YWGPIO_Read ( YWGPIO_Handle_T GpioHandle,
                                                    U8* PioValue );
YW_ErrorType_T YWGPIO_Write ( YWGPIO_Handle_T GpioHandle,
                                                   U8 PioValue );
YW_ErrorType_T YWGPIO_RegisterISR( YWGPIO_Handle_T GpioHandle,
                                                        YWGPIO_ISRFunc_T  ISR,
                                                         U32 Priority);
YW_ErrorType_T YWGPIO_UnRegisterISR( YWGPIO_Handle_T GpioHandle);
YW_ErrorType_T YWGPIO_EnableInterrupt( YWGPIO_Handle_T GpioHandle,
                                                            YWGPIO_InterruptMode_T INTMode);
YW_ErrorType_T YWGPIO_DisableInterrupt( YWGPIO_Handle_T GpioHandle);
U32              YWGpioO_GetVersion( S8 *pchVer, U32 nSize  );


#ifdef __cplusplus
}
#endif

#endif
