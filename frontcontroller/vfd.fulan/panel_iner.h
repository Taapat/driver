
#ifndef __PANEL_INER_H
#define __PANEL_INER_H

#ifdef __cplusplus
extern "C" {
#endif

#define	REMOTE_SLAVE_ADDRESS			0x40bd0000	/* slave address is 5 */
#define	REMOTE_SLAVE_ADDRESS_NEW	0xc03f0000	/*sz 2008-06-26 add new remote*/
#define	REMOTE_SLAVE_ADDRESS_EDISION1	0x22dd0000
#define	REMOTE_SLAVE_ADDRESS_EDISION2	0XCC330000
#define	REMOTE_SLAVE_ADDRESS_GOLDEN 	0x48b70000	/* slave address is 5 */
#define REMOTE_TOPFIELD_MASK			0x4fb0000

#define YW_VFD_ENABLE
#define  INVALID_KEY		-1

#define	I2C_BUS_NUM		1
#define	I2C_BUS_ADD		(0x50>>1)  //this is important not 0x50

typedef   unsigned int		   YWOS_ClockMsec;

#define YW_PANEL_DEBUG
#ifdef YW_PANEL_DEBUG
#define PANEL_DEBUG(x)	ywtrace_print(TRACE_ERROR,"FUNC %s error for at %d in %s ^!^\n",__FUNCTION__, __LINE__, __FILE__)
#define PANEL_PRINT(x)	ywtrace_print x
#else
#define PANEL_DEBUG(x) 
#define PANEL_PRINT(x)
#endif 
/********************************  数据结构************************************/

typedef enum 
{
	REMOTE_OLD,
	REMOTE_NEW,
	REMOTE_TOPFIELD,
	REMOTE_EDISION1,
	REMOTE_EDISION2,
	REMOTE_GOLDEN,
	REMOTE_UNKNOWN
}REMOTE_TYPE;

typedef enum VFDMode_e{
	VFDWRITEMODE,
	VFDREADMODE
}VFDMode_T;
 
typedef enum SegNum_e{
	SEGNUM1 = 0,
	SEGNUM2
}SegNum_T;

typedef struct SegAddrVal_s{
	u8 Segaddr1;
	u8 Segaddr2;
	u8 CurrValue1;
	u8 CurrValue2;
}SegAddrVal_T;

#define YWPANEL_FP_INFO_MAX_LENGTH		  (10) 			/*读指令最大长度*/
#define YWPANEL_FP_DATA_MAX_LENGTH		  (38)			/*写指令最大长度*/


typedef struct YWPANEL_I2CData_s
{
	u8	writeBuffLen;
	u8	writeBuff[YWPANEL_FP_DATA_MAX_LENGTH];
	u8	readBuff[YWPANEL_FP_INFO_MAX_LENGTH]; /*I2C返回的缓冲*/

}YWPANEL_I2CData_t;


typedef enum YWPANEL_DataType_e
{
	YWPANEL_DATATYPE_LBD = 0x01,
	YWPANEL_DATATYPE_LCD,
	YWPANEL_DATATYPE_LED,
	YWPANEL_DATATYPE_VFD,
	YWPANEL_DATATYPE_SCANKEY,
	YWPANEL_DATATYPE_IRKEY,

	YWPANEL_DATATYPE_GETVERSION,
	YWPANEL_DATATYPE_GETVFDSTATE,
	YWPANEL_DATATYPE_SETVFDSTATE,
	YWPANEL_DATATYPE_GETCPUSTATE,
	YWPANEL_DATATYPE_SETCPUSTATE,
	
	YWPANEL_DATATYPE_GETSTBYKEY1,
	YWPANEL_DATATYPE_GETSTBYKEY2,
	YWPANEL_DATATYPE_GETSTBYKEY3,
	YWPANEL_DATATYPE_GETSTBYKEY4,
	YWPANEL_DATATYPE_GETSTBYKEY5,
	YWPANEL_DATATYPE_SETSTBYKEY1,
	YWPANEL_DATATYPE_SETSTBYKEY2,
	YWPANEL_DATATYPE_SETSTBYKEY3,
	YWPANEL_DATATYPE_SETSTBYKEY4,
	YWPANEL_DATATYPE_SETSTBYKEY5,

	YWPANEL_DATATYPE_GETIRCODE,
	YWPANEL_DATATYPE_SETIRCODE,

	YWPANEL_DATATYPE_GETENCRYPTMODE,	/*读取加密模式*/
	YWPANEL_DATATYPE_SETENCRYPTMODE,	/*设置加密模式*/
	YWPANEL_DATATYPE_GETENCRYPTKEY, 	  /*读取加密密钥*/
	YWPANEL_DATATYPE_SETENCRYPTKEY, 	  /*设置加密密钥*/	

	YWPANEL_DATATYPE_GETVERIFYSTATE,	 /*读取校验状态*/
	YWPANEL_DATATYPE_SETVERIFYSTATE,	 /*读取校验状态*/

	YWPANEL_DATATYPE_GETTIME,					 /*读取面板时间*/
	YWPANEL_DATATYPE_SETTIME,					 /*设置面板时间*/
	YWPANEL_DATATYPE_CONTROLTIMER,			/*控制计时器开始计时or 停止计时*/
	
	YWPANEL_DATATYPE_SETPOWERONTIME,		   /*设置待机时间(设置为0xffffffff时为关闭计时)*/ 
	YWPANEL_DATATYPE_GETPOWERONTIME,

	YWPANEL_DATATYPE_GETVFDSTANDBYSTATE,	   
	YWPANEL_DATATYPE_SETVFDSTANDBYSTATE,

	YWPANEL_DATATYPE_GETBLUEKEY1,
	YWPANEL_DATATYPE_GETBLUEKEY2,
	YWPANEL_DATATYPE_GETBLUEKEY3,
	YWPANEL_DATATYPE_GETBLUEKEY4,
	YWPANEL_DATATYPE_GETBLUEKEY5,
	YWPANEL_DATATYPE_SETBLUEKEY1,
	YWPANEL_DATATYPE_SETBLUEKEY2,
	YWPANEL_DATATYPE_SETBLUEKEY3,
	YWPANEL_DATATYPE_SETBLUEKEY4,
	YWPANEL_DATATYPE_SETBLUEKEY5,

	YWPANEL_DATATYPE_GETPOWERONSTATE,	   /*0x77*/
	YWPANEL_DATATYPE_SETPOWERONSTATE,	   /*0x78*/

	YWPANEL_DATATYPE_GETSTARTUPSTATE,	   /*0x79*/

	YWPANEL_DATATYPE_GETLOOPSTATE,		/*0x80*/
	YWPANEL_DATATYPE_SETLOOPSTATE,		/*0x81*/

	YWPANEL_DATATYPE_NUM
}YWPANEL_DataType_t;

typedef struct YWPANEL_LBDData_s
{
	u8	value; /*8位分别代表各个灯*/
}YWPANEL_LBDData_t;

typedef struct YWPANEL_LEDData_s
{
	u8	led1; 
	u8	led2;
	u8	led3;
	u8	led4;
}YWPANEL_LEDData_t;

typedef struct YWPANEL_LCDData_s
{
	u8	value; 
}YWPANEL_LCDData_t;

typedef enum YWPANEL_VFDDataType_e
{
	YWPANEL_VFD_SETTING = 0x1,
	YWPANEL_VFD_DISPLAY,
	YWPANEL_VFD_READKEY,
	YWPANEL_VFD_DISPLAYSTRING

}YWPANEL_VFDDataType_t;


typedef struct YWPANEL_VFDData_s
{
	YWPANEL_VFDDataType_t  type; /*1- setting  2- display 3- readscankey  4-displaystring*/ 

	u8 setValue;   //if type == YWPANEL_VFD_SETTING
	
	u8 address[16];  //if type == YWPANEL_VFD_DISPLAY
	u8 DisplayValue[16];

	u8 key;  //if type == YWPANEL_VFD_READKEY
	
}YWPANEL_VFDData_t;

typedef struct YWPANEL_IRKEY_s
{
	u32 customCode;
	u32 dataCode;
}YWPANEL_IRKEY_t;

typedef struct YWPANEL_SCANKEY_s
{
	u32 keyValue;
}YWPANEL_SCANKEY_t;

typedef struct YWPANEL_StandByKey_s
{
	u32 	key; 
	
}YWPANEL_StandByKey_t;

typedef enum YWPANEL_IRCODE_e
{
	YWPANEL_IRCODE_NONE,
	YWPANEL_IRCODE_NEC = 0x01,
	YWPANEL_IRCODE_RC5,
	YWPANEL_IRCODE_RC6,
	YWPANEL_IRCODE_PILIPS
}YWPANEL_IRCODE_T;

typedef struct YWPANEL_IRCode_s
{
	YWPANEL_IRCODE_T	code;
}YWPANEL_IRCode_t;


typedef enum YWPANEL_ENCRYPEMODE_e
{
	YWPANEL_ENCRYPEMODE_NONE =0x00,
	YWPANEL_ENCRYPEMODE_ODDBIT, 			/*奇位取反*/
	YWPANEL_ENCRYPEMODE_EVENBIT,			/*偶位取反*/
	YWPANEL_ENCRYPEMODE_RAMDONBIT , 	/*随机位取反*/
}YWPANEL_ENCRYPEMODE_T;


typedef struct YWPANEL_EncryptMode_s
{
	YWPANEL_ENCRYPEMODE_T	 mode;
	
}YWPANEL_EncryptMode_t;

typedef struct YWPANEL_EncryptKey_s
{
	u32 	  key;
}YWPANEL_EncryptKey_t;

typedef enum YWPANEL_VERIFYSTATE_e
{
	YWPANEL_VERIFYSTATE_NONE =0x00,
	YWPANEL_VERIFYSTATE_CRC16 ,
	YWPANEL_VERIFYSTATE_CRC32 ,
	YWPANEL_VERIFYSTATE_CHECKSUM ,

}YWPANEL_VERIFYSTATE_T;

typedef struct YWPANEL_VerifyState_s
{
	YWPANEL_VERIFYSTATE_T		state;
}YWPANEL_VerifyState_t;

typedef struct YWPANEL_Time_s
{
	u32 	  second;						/*从1970年1月1日00点00分开始的秒数*/
}YWPANEL_Time_t;

typedef struct YWPANEL_ControlTimer_s
{
	bool			startFlag;							// 0 - stop  1-start					
}YWPANEL_ControlTimer_t;

typedef struct YWPANEL_VfdStandbyState_s
{
	bool			On; 						 // 0 - off  1-on					 
}YWPANEL_VfdStandbyState_T;

typedef struct YWPANEL_BlueKey_s
{
	u32 	key; 
}YWPANEL_BlueKey_t;

typedef struct YWPANEL_PowerOnState_s
{
	YWPANEL_POWERONSTATE_t state; 
	
}YWPANEL_PowerOnState_t;

typedef struct YWPANEL_StartUpState_s
{
	YWPANEL_STARTUPSTATE_t State; 
	
}YWPANEL_StartUpState_t;

typedef struct YWPANEL_LoopState_s
{
	YWPANEL_LOOPSTATE_t state; 
	
}YWPANEL_LoopState_t;

typedef enum YWPANEL_LBDType_e
{
	YWPANEL_LBD_TYPE_POWER		  =  ( 1 << 0 ),			/*	前面板Power灯 */
	YWPANEL_LBD_TYPE_SIGNAL 	   =  ( 1 << 1 ) ,			 /*  前面板 Signal灯 */
	YWPANEL_LBD_TYPE_MAIL		 =	( 1 << 2 ), 		   /*  前面板Mail灯 */
	YWPANEL_LBD_TYPE_AUDIO		  =  ( 1 << 3 ) 		/*	前面板Audio灯 */
}YWPANEL_LBDType_T;

typedef enum YWPAN_FP_MCUTYPE_E
{
	YWPANEL_FP_MCUTYPE_UNKNOW = 0x00,
	YWPANEL_FP_MCUTYPE_AVR_ATTING48,	   //AVR MCU
	YWPANEL_FP_MCUTYPE_AVR_ATTING88,

	YWPAN_FP_MCUTYPE_MAX 
}YWPAN_FP_MCUTYPE_T;

typedef	enum YWPANEL_FP_DispType_e
{
	YWPANEL_FP_DISPTYPE_UNKNOWN = 0x00,
	YWPANEL_FP_DISPTYPE_VFD = (1 << 0),
	YWPANEL_FP_DISPTYPE_LCD = (1 << 1),
	YWPANEL_FP_DISPTYPE_LED = (1 << 2),
	YWPANEL_FP_DISPTYPE_LBD = (1 << 3)
}YWPANEL_FP_DispType_t;

typedef struct YWPANEL_Version_s
{
	YWPAN_FP_MCUTYPE_T	CpuType;
	
	u8	DisplayInfo;
	u8	scankeyNum;
	u8	swMajorVersion; 	
	u8	swSubVersion;
	
}YWPANEL_Version_t;

typedef struct YWPANEL_FPData_s
{
	YWPANEL_DataType_t			dataType;

	union
	{
		YWPANEL_Version_t		version;   
		YWPANEL_LBDData_t		lbdData;
		YWPANEL_LEDData_t		ledData;
		YWPANEL_LCDData_t		lcdData;
		YWPANEL_VFDData_t		vfdData;
		YWPANEL_IRKEY_t 		IrkeyData;
		YWPANEL_SCANKEY_t		ScanKeyData;
		YWPANEL_CpuState_t		CpuState;
		YWPANEL_StandByKey_t		stbyKey;
		YWPANEL_IRCode_t		irCode;
		YWPANEL_EncryptMode_t		EncryptMode;
		YWPANEL_EncryptKey_t		EncryptKey;
		YWPANEL_VerifyState_t		verifyState;
		YWPANEL_Time_t			time;
		YWPANEL_ControlTimer_t		TimeState;
		YWPANEL_VfdStandbyState_T	VfdStandbyState;
		YWPANEL_BlueKey_t		BlueKey;
		YWPANEL_PowerOnState_t		PowerOnState;
		YWPANEL_StartUpState_t		StartUpState;
		YWPANEL_LoopState_t 		LoopState;
	}data;

	bool	ack;

}YWPANEL_FPData_t;

int 	YWPANEL_VFD_DETECT(void);

int		YWPANEL_VFD_Init(void);
int		YWPANEL_VFD_Term(void); 

int YWPANEL_VFD_GetRevision(char * version);
int YWPANEL_VFD_ShowIco(LogNum_T log_num,int log_stat);
int YWPANEL_VFD_ShowString(char* str);
int YWVFD_LED_ShowString(const char *str); //lwj add
int YWPANEL_VFD_ShowTime(u8 hh,u8 mm);
int YWPANEL_VFD_ShowTimeOff(void);
int YWPANEL_VFD_SetBrightness(int level);
YWPANEL_VFDSTATE_t YWPANEL_FP_GetVFDStatus(void);
bool  YWPANEL_FP_SetVFDStatus(YWPANEL_VFDSTATE_t state);
YWPANEL_CPUSTATE_t YWPANEL_FP_GetCpuStatus(void);
bool  YWPANEL_FP_SetCpuStatus(YWPANEL_CPUSTATE_t state);
bool  YWPANEL_FP_ControlTimer(bool on);
YWPANEL_POWERONSTATE_t YWPANEL_FP_GetPowerOnStatus(void);
bool  YWPANEL_FP_SetPowerOnStatus(YWPANEL_POWERONSTATE_t state);
u32  YWPANEL_FP_GetTime(void);
bool  YWPANEL_FP_SetTime(u32 value);
u32  YWPANEL_FP_GetStandByKey(u8 index);
bool  YWPANEL_FP_SetStandByKey(u8 index,u8 key);
u32  YWPANEL_FP_GetBlueKey(u8 index);
bool  YWPANEL_FP_SetBlueKey(u8 index,u8 key);
int YWPANEL_LBD_SetStatus(YWPANEL_LBDStatus_T  LBDStatus );
bool YWPANEL_FP_GetStartUpState(YWPANEL_STARTUPSTATE_t *State);

//u32  YWPANEL_FP_GetIRKey(void);
bool  YWPANEL_FP_SetPowerOnTime(u32 Value);
u32  YWPANEL_FP_GetPowerOnTime(void);
int YWPANEL_VFD_GetKeyValue(void);

#ifdef __cplusplus
}
#endif


#endif	/* __PANEL_INER_H */
/* EOF------------------------------------------------------------------------*/
