#ifndef __RTL871X_PWRCTRL_H_
#define __RTL871X_PWRCTRL_H_

#include <drv_conf.h>
#include <osdep_service.h>		
#include <drv_types.h>


#define FW_PWR0	0	
#define FW_PWR1 	1
#define FW_PWR2 	2
#define FW_PWR3 	3


#define HW_PWR0	7	
#define HW_PWR1 	6
#define HW_PWR2 	2
#define HW_PWR3	0
#define HW_PWR4	8

#define FW_PWRMSK	0x7


#define XMIT_ALIVE	BIT(0)
#define RECV_ALIVE	BIT(1)
#define CMD_ALIVE	BIT(2)
#define EVT_ALIVE	BIT(3)


enum Power_Mgnt
{
	PS_MODE_ACTIVE	= 0	,
	PS_MODE_MIN			,
	PS_MODE_MAX			,
	PS_MODE_DTIM			,
	PS_MODE_VOIP			,
	PS_MODE_UAPSD_WMM	,
	PS_MODE_UAPSD			,
	PS_MODE_IBSS			,
	PS_MODE_WWLAN		,
	PM_Radio_Off			,
	PM_Card_Disable		,
	PS_MODE_NUM
};


/*
	BIT[2:0] = HW state
	BIT[3] = Protocol PS state,   0: register active state , 1: register sleep state
	BIT[4] = sub-state
*/

#define 	PS_DPS				BIT(0)
#define 	PS_LCLK				(PS_DPS)
#define	PS_RF_OFF			BIT(1)
#define	PS_ALL_ON			BIT(2)
#define	PS_ST_ACTIVE		BIT(3)
#define	PS_LP				BIT(4)	// low performance

#define	PS_STATE_MASK		(0x0F)
#define	PS_STATE_HW_MASK	(0x07)
#define 	PS_SEQ_MASK		(0xc0)

#define	PS_STATE(x)			(PS_STATE_MASK & (x))
#define	PS_STATE_HW(x)	(PS_STATE_HW_MASK & (x))
#define	PS_SEQ(x)			(PS_SEQ_MASK & (x))

#define	PS_STATE_S0		(PS_DPS)
#define 	PS_STATE_S1		(PS_LCLK)
#define	PS_STATE_S2		(PS_RF_OFF)
#define 	PS_STATE_S3		(PS_ALL_ON)
#define	PS_STATE_S4		((PS_ST_ACTIVE) | (PS_ALL_ON))


#define 	PS_IS_RF_ON(x)		((x) & (PS_ALL_ON))
#define 	PS_IS_ACTIVE(x)		((x) & (PS_ST_ACTIVE))
#define 	CLR_PS_STATE(x)	((x) = ((x) & (0xF0)))


struct reportpwrstate_parm {
	unsigned char mode;
	unsigned char state; //the CPWM value
	unsigned short rsvd;
}; 


typedef _sema _pwrlock;


static void __inline _init_pwrlock(_pwrlock *plock)
{
	_init_sema(plock, 1);
}

static void __inline _free_pwrlock(_pwrlock *plock)
{
	_free_sema(plock);
}


static void __inline _enter_pwrlock(_pwrlock *plock)
{
	_down_sema(plock);
}


static void __inline _exit_pwrlock(_pwrlock *plock)
{
	_up_sema(plock);
}

#define LPS_DELAY_TIME	1*HZ // 1 sec

#define EXE_PWR_NONE	0x01
#define EXE_PWR_IPS		0x02
#define EXE_PWR_LPS		0x04

// RF state.
typedef enum _rt_rf_power_state
{
	rf_on,		// RF is on after RFSleep or RFOff
	rf_sleep,	// 802.11 Power Save mode
	rf_off,		// HW/SW Radio OFF or Inactive Power Save
	//=====Add the new RF state above this line=====//
	rf_max
}rt_rf_power_state;

struct	pwrctrl_priv {
	_pwrlock	lock;
	volatile u8 rpwm; // requested power state for fw
	volatile u8 cpwm; // fw current power state. updated when 1. read from HCPWM 2. driver lowers power level
	volatile u8 tog; // toggling
	volatile u8 cpwm_tog; // toggling
	uint pwr_mode;
	uint smart_ps;
	uint alives;


	_timer 	pwr_state_check_timer;

	//u8	ips_enable;//for dbg
	//u8	lps_enable;//for dbg
	
	uint 	bips_processing;
	rt_rf_power_state	inactive_pwrstate;
	rt_rf_power_state	change_pwrstate;
	uint 	ips_enter_cnts;
	uint 	ips_leave_cnts;
		
	_workitem InactivePSWorkItem;
	_timer 	ips_check_timer;


	u8	bLeisurePs;
	u8	LpsIdleCount;
	//u8	FWCtrlPSMode;
	u8	power_mgnt;
	u8	bFwCurrentInPSMode;
	u32	DelayLPSLastTimeStamp;

	s32		pnp_current_pwr_state;
	u8		pnp_bstop_trx;
};



extern void init_pwrctrl_priv(_adapter *adapter);
extern void free_pwrctrl_priv(_adapter * adapter);
extern sint register_tx_alive(_adapter *padapter);
extern void unregister_tx_alive(_adapter *padapter);
extern sint register_rx_alive(_adapter *padapter);
extern void unregister_rx_alive(_adapter *padapter);
extern sint register_cmd_alive(_adapter *padapter);
extern void unregister_cmd_alive(_adapter *padapter);
extern sint register_evt_alive(_adapter *padapter);
extern void unregister_evt_alive(_adapter *padapter);
extern void cpwm_int_hdl(_adapter *padapter, struct reportpwrstate_parm *preportpwrstate);
extern void set_ps_mode(_adapter * padapter, uint ps_mode, uint smart_ps);
extern void set_rpwm(_adapter * padapter, u8 val8);
extern void LeaveAllPowerSaveMode(PADAPTER Adapter);
void ips_enter(_adapter * padapter);
int ips_leave(_adapter * padapter);

#ifdef CONFIG_LPS
void LPS_Enter(PADAPTER padapter);
void LPS_Leave(PADAPTER padapter);
u8 FWLPS_RF_ON(PADAPTER padapter);
#endif

#endif  //__RTL871X_PWRCTRL_H_
