/**********************************文件头部注释************************************/
//
//
//                      Copyright (C), 2005-2010, AV Frontier Tech. Co., Ltd.
//
//
// 文件名：      ywi2c_soft.h
//
// 创建者：      chenj
//
// 创建时间：    2011.02.10
//
// 文件描述：    模拟I2C驱动
//
// 修改记录：   日       期      作      者       版本      修定
//              ---------        ---------        -----     -----
//              2011.02.10       chenj            0.01      新建
/*****************************************************************************************/

#ifndef __YWI2C_SOFT_H__
#define __YWI2C_SOFT_H__


#ifdef __cplusplus
extern "C" {
#endif


typedef U32     YWI2CSoft_Handle_t;



typedef struct YWI2cSoft_InitParam_s
{

    BOOL    IsSlaveDevice;

    U32     SCLPioIndex;
    U32     SDAPioIndex;

    U32     Speed;
}YWI2cSoft_InitParam_t;



typedef struct YWI2cSoft_OpenParams_s
{
    U16              I2cAddress;

} YWI2cSoft_OpenParams_t;



/* C++ support */
/* ----------- */
#ifdef __cplusplus
}
#endif




#endif

