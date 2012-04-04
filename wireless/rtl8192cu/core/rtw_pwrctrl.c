/******************************************************************************
* rtl871x_pwrctrl.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL871X_PWRCTRL_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>

#ifdef CONFIG_SDIO_HCI
#ifdef PLATFORM_LINUX
        #include<linux/mmc/sdio_func.h>
#endif
#include <sdio_ops.h>
#endif

#ifdef CONFIG_IPS

extern int r871xu_ips_pwr_up(_adapter *padapter);
extern void r871xu_ips_pwr_down(_adapter *padapter);

#ifdef PLATFORM_LINUX
void pwr_state_check_handler(void *FunctionContext)
{
	_adapter *padapter = (_adapter *)FunctionContext;
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	struct pwrctrl_priv *pwrctrlpriv = &padapter->pwrctrlpriv;
	
	
	//if(!pwrctrlpriv->ips_enable)
	//	return;
	
	if(padapter->net_closed == _TRUE)
			return;
	
	//printk("%s\n",__FUNCTION__);
	if (	(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||	
		(check_fwstate(pmlmepriv, _FW_LINKED|_FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE)  ||
		(padapter->net_closed == _TRUE)|| (padapter->bup == _FALSE)		
		)
	{
	
		//other pwr ctrl....	
		_set_timer(&padapter->pwrctrlpriv.pwr_state_check_timer, 2000);
	}	
	else
	{
	
		printk("==>pwr_state_check_handler .rf_state(%x),processing(%x)\n",pwrpriv->inactive_pwrstate,pwrpriv->bips_processing);
		printk("IPS......rf_state(%x),processing(%x)\n",pwrpriv->inactive_pwrstate,pwrpriv->bips_processing);
		if((rf_on == pwrpriv->inactive_pwrstate) &&(pwrpriv->bips_processing == _FALSE))
		{
			ips_enter(padapter);
		}		
		
	}
	

}


void InactivePSWorkItemCallback(struct work_struct *work)
{
	struct pwrctrl_priv *pwrpriv = container_of(work, struct pwrctrl_priv, InactivePSWorkItem);
	_adapter *padapter = container_of(pwrpriv, _adapter, pwrctrlpriv);
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	if (	(check_fwstate(pmlmepriv, _FW_LINKED|_FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(padapter->net_closed == _TRUE)
	)
	{
		return;
	}
_enter_pwrlock(&pwrpriv->lock);
	pwrpriv->bips_processing = _TRUE;
		
	if(rf_off == pwrpriv->change_pwrstate )
	{	
		printk("==>InactivePSWorkItemCallback change rf to OFF...LED(0x%08x).... \n\n",read32(padapter,0x4c));
		r871xu_ips_pwr_down(padapter);
		pwrpriv->inactive_pwrstate = rf_off;
	}
	
	pwrpriv->bips_processing = _FALSE;
_exit_pwrlock(&pwrpriv->lock);
}
#endif
void ips_enter(_adapter * padapter)
{
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	
	if (	(check_fwstate(pmlmepriv, _FW_LINKED|_FW_UNDER_SURVEY|_FW_UNDER_LINKING) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) ||
		(padapter->net_closed == _TRUE)
	)
	{
		return;
	}
	if((pwrpriv->inactive_pwrstate == rf_on) &&(_FALSE == pwrpriv->bips_processing)){
		pwrpriv->change_pwrstate = rf_off;
		pwrpriv->ips_enter_cnts++;
		printk("==>ips_enter cnts:%d\n",pwrpriv->ips_enter_cnts);
		_set_workitem(&(pwrpriv->InactivePSWorkItem));
	}
	
}

int ips_leave(_adapter * padapter)
{
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;
	struct security_priv* psecuritypriv=&(padapter->securitypriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	int result = _TRUE;
	sint keyid;
	_enter_pwrlock(&pwrpriv->lock);
	if((pwrpriv->inactive_pwrstate == rf_off) &&(!pwrpriv->bips_processing))
	{
		pwrpriv->change_pwrstate = rf_on;
		pwrpriv->ips_leave_cnts++;
		printk("==>ips_leave cnts:%d\n",pwrpriv->ips_leave_cnts);
#if 0
		//_set_workitem(&(pwrpriv->InactivePSWorkItem));
#else
		pwrpriv->bips_processing = _TRUE;		

		result = r871xu_ips_pwr_up(padapter);		
		pwrpriv->inactive_pwrstate = rf_on;

		if((_WEP40_ == psecuritypriv->dot11PrivacyAlgrthm) ||(_WEP104_ == psecuritypriv->dot11PrivacyAlgrthm))
		{
			printk("==>%s,channel(%d)\n",__FUNCTION__,pmlmeext->cur_channel);
			set_channel_bwmode(padapter, pmlmeext->cur_channel, HAL_PRIME_CHNL_OFFSET_DONT_CARE, HT_CHANNEL_WIDTH_20);			
			for(keyid=0;keyid<4;keyid++){
				if(pmlmeinfo->key_mask & BIT(keyid)){
					result=set_key(padapter,psecuritypriv, keyid);	
				}
			}
		}
		
		printk("==> ips_leave.....LED(0x%08x)...\n",read32(padapter,0x4c));
		pwrpriv->bips_processing = _FALSE;
#endif
	}
	_exit_pwrlock(&pwrpriv->lock);
	return result;
}


#endif

#ifdef CONFIG_LPS
void set_rpwm(_adapter * padapter, u8 val8)
{
	u8	rpwm;
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;

_func_enter_;

	if(pwrpriv->rpwm == val8){
		RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("Already set rpwm [%d] ! \n", val8));
		return;
	}

	if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE)){
		RT_TRACE(_module_rtl871x_xmit_c_,_drv_err_,("set_rpwm=> bDriverStopped or bSurpriseRemoved \n"));
		return;
	}
	rpwm = val8 |pwrpriv->tog;

	pwrpriv->rpwm = val8;
	
	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("set_rpwm: value = %x\n", rpwm));

	write8(padapter, REG_USB_HRPWM, rpwm);
	
	pwrpriv->tog += 0x80;

_func_exit_;
}

u8 PS_RDY_CHECK(_adapter * padapter)
{
	u32 curr_time, delta_time;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);

	curr_time = get_current_time();	

	delta_time = curr_time -pwrpriv->DelayLPSLastTimeStamp;

	if(delta_time < LPS_DELAY_TIME)
	{		
		return _FALSE;
	}

	if (	(check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE) ||
		(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) )
		return _FALSE;

	if( (padapter->securitypriv.dot11AuthAlgrthm == dot11AuthAlgrthm_8021X) && (padapter->securitypriv.binstallGrpkey == _FALSE) )
	{
		DBG_8192C("Group handshake still in progress !!!\n");
		return _FALSE;
	}

	return _TRUE;
}

void set_ps_mode(_adapter * padapter, uint ps_mode, uint smart_ps)
{
	struct pwrctrl_priv *pwrpriv = &padapter->pwrctrlpriv;

_func_enter_;

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("========= Power Mode is :%d, Smart_PS = %d\n", ps_mode,smart_ps));
	//printk("========= Power Mode is :%d, Smart_PS = %d\n", ps_mode,smart_ps);

	if(ps_mode > PM_Card_Disable) {
		RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("ps_mode:%d error\n", ps_mode));
		return;
	}

	if(pwrpriv->pwr_mode == ps_mode){
		return;
	}

	pwrpriv->pwr_mode = ps_mode;

	if(pwrpriv->pwr_mode == PS_MODE_ACTIVE)
	{
		set_rpwm(padapter, PS_STATE_S4);
		set_FwPwrMode_cmd(padapter, ps_mode);
		pwrpriv->bFwCurrentInPSMode = _FALSE;
	}
	else
	{
		if(PS_RDY_CHECK(padapter))
		{
			pwrpriv->bFwCurrentInPSMode = _TRUE;
			set_FwPwrMode_cmd(padapter, ps_mode);
			set_rpwm(padapter, PS_STATE_S2);
		}
		else
		{
			pwrpriv->pwr_mode = PS_MODE_ACTIVE;
		}
	}

_func_exit_;
}


//
//	Description:
//		Enter the leisure power save mode.
//
void LPS_Enter(PADAPTER padapter)
{
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);

_func_enter_;

	//printk("LeisurePSEnter()...\n");
	
	if (	(check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE) ||
		(check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_AP_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE) )
		return;

	if (pwrpriv->bLeisurePs)
	{
		// Idle for a while if we connect to AP a while ago.
		if(pwrpriv->LpsIdleCount >= 2) //  4 Sec 
		{
			if(pwrpriv->pwr_mode == PS_MODE_ACTIVE)
			{
				//printk("LeisurePSEnter(): Enter 802.11 power save mode...\n");

				set_ps_mode(padapter, pwrpriv->power_mgnt, 1);
			}	
		}
		else
			pwrpriv->LpsIdleCount++;
	}

_func_exit_;
}


//
//	Description:
//		Leave the leisure power save mode.
//
void LPS_Leave(PADAPTER padapter)
{
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);

_func_enter_;

	//printk("LeisurePSLeave()...\n");

	if (pwrpriv->bLeisurePs)
	{	
		if(pwrpriv->pwr_mode != PS_MODE_ACTIVE)
		{
			printk("LeisurePSLeave(): Busy Traffic , Leave 802.11 power save..\n");
			set_ps_mode(padapter, PS_MODE_ACTIVE, 0);
		}
	}

_func_exit_;
}

u8 FWLPS_RF_ON(PADAPTER padapter)
{
	u32	valRCR;
	u8	ret;

_func_enter_;

	if(padapter->pwrctrlpriv.inactive_pwrstate == rf_off)
	{
		// If it is in HW/SW Radio OFF or IPS state, we do not check Fw LPS Leave,
		// because Fw is unload.
		ret = _TRUE;
	}
	else
	{
		valRCR = read32(padapter, REG_RCR);
		valRCR &= 0x00070000;
		if(valRCR)
			ret = _FALSE;
		else
			ret = _TRUE;
	}

_func_exit_;

	return ret;
}
#endif

//
// Description: Leave all power save mode: LPS, FwLPS, IPS if needed.
// Move code to function by tynli. 2010.03.26. 
//
void LeaveAllPowerSaveMode(IN PADAPTER Adapter)
{
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	u32 LPSLeaveTimeOut = 10000;
	//u32 IPSLeaveTimeOut = 10000;

_func_enter_;
	printk("%s.....\n",__FUNCTION__);
	if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)
	{ //connect
#ifdef CONFIG_LPS
		printk("==> leave LPS.......\n");
		LPS_Leave(Adapter);

		while( !FWLPS_RF_ON(Adapter) )
		{
			usleep_os(100);
			LPSLeaveTimeOut--;
			if(LPSLeaveTimeOut <= 0)
			{
				printk("Wait for FW LPS leave too long!!! LPSLeaveTimeOut  = %d\n", LPSLeaveTimeOut );
				break;
			}
		}
#endif
	}
	else
	{
#ifdef CONFIG_IPS
		printk("==> leave IPS.......\n");
		if(Adapter->pwrctrlpriv.inactive_pwrstate== rf_off)
		{				
			if(_FALSE == ips_leave(Adapter))
			{
				printk("======> ips_leave fail.............\n");			
			}
		}	
		
#endif
	}

_func_exit_;
}

#ifdef CONFIG_PWRCTRL

/*
Caller:ISR handler...

This will be called when CPWM interrupt is up.

using to update cpwn of drv; and drv willl make a decision to up or down pwr level
*/
void cpwm_int_hdl(_adapter *padapter, struct reportpwrstate_parm *preportpwrstate)
{
	struct pwrctrl_priv *pwrpriv = &(padapter->pwrctrlpriv);
	struct cmd_priv	*pcmdpriv = &(padapter->cmdpriv);
	struct xmit_priv	*pxmitpriv = &(padapter->xmitpriv);

_func_enter_;

	if(pwrpriv->cpwm_tog == ((preportpwrstate->state)&0x80)){
		RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("cpwm_int_hdl : cpwm_tog = %x this time cpwm=0x%x  toggle bit didn't change !!!\n",pwrpriv->cpwm_tog ,preportpwrstate->state));	
		goto exit;
	}

	_enter_pwrlock(&pwrpriv->lock);

	pwrpriv->cpwm = (preportpwrstate->state)&0xf;

	if(pwrpriv->cpwm >= PS_STATE_S2){
		if(pwrpriv->alives & CMD_ALIVE)
			_up_sema(&(pcmdpriv->cmd_queue_sema));

		if(pwrpriv->alives & XMIT_ALIVE)
			_up_sema(&(pxmitpriv->xmit_sema));
	}
	pwrpriv->cpwm_tog=  (preportpwrstate->state)&0x80;
	_exit_pwrlock(&pwrpriv->lock);
exit:
	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("cpwm_int_hdl : cpwm = %x !!!\n",pwrpriv->cpwm));

_func_exit_;

}


static __inline void	register_task_alive(struct pwrctrl_priv *pwrctrl, uint tag)
{
_func_enter_;
		pwrctrl->alives |= tag;
_func_exit_;
}

static __inline void	unregister_task_alive(struct pwrctrl_priv *pwrctrl, uint tag)
{
_func_enter_;

	if (pwrctrl->alives & tag)
		pwrctrl->alives ^= tag;

_func_exit_;	
}
#endif


void	init_pwrctrl_priv(_adapter *padapter)
{
	struct pwrctrl_priv *pwrctrlpriv = &padapter->pwrctrlpriv;
	struct registry_priv* pregistrypriv = &padapter->registrypriv;

_func_enter_;

	_memset((unsigned char *)pwrctrlpriv, 0, sizeof(struct pwrctrl_priv));

#ifdef PLATFORM_WINDOWS
	pwrctrlpriv->pnp_current_pwr_state=NdisDeviceStateD0;
#endif

	_init_pwrlock(&pwrctrlpriv->lock);
	//pwrctrlpriv->ips_enable = pregistrypriv->ips_enable;	
	//pwrctrlpriv->lps_enable = pregistrypriv->lps_enable;	

	pwrctrlpriv->inactive_pwrstate = rf_on;
	pwrctrlpriv->ips_enter_cnts=0;
	pwrctrlpriv->ips_leave_cnts=0;
	
	pwrctrlpriv->LpsIdleCount = 0;
	//pwrctrlpriv->FWCtrlPSMode =padapter->registrypriv.power_mgnt;// PS_MODE_MIN;
	pwrctrlpriv->power_mgnt =padapter->registrypriv.power_mgnt;// PS_MODE_MIN;
	pwrctrlpriv->bLeisurePs = (PS_MODE_ACTIVE != pwrctrlpriv->power_mgnt)?_TRUE:_FALSE;
	
	pwrctrlpriv->bFwCurrentInPSMode = _FALSE;

	pwrctrlpriv->cpwm = PS_STATE_S4;

	pwrctrlpriv->pwr_mode = PS_MODE_ACTIVE;

	
	pwrctrlpriv->smart_ps = 0;

	pwrctrlpriv->tog = 0x80;

#ifdef CONFIG_IPS
	_init_workitem(&(pwrctrlpriv->InactivePSWorkItem), InactivePSWorkItemCallback, padapter);

#ifdef PLATFORM_LINUX		
	_init_timer(&(pwrctrlpriv->pwr_state_check_timer), padapter->pnetdev, pwr_state_check_handler, (u8 *)padapter);

#endif

#endif

_func_exit_;

}


void	free_pwrctrl_priv(_adapter *adapter)
{
	struct pwrctrl_priv *pwrctrlpriv = &adapter->pwrctrlpriv;

_func_enter_;

	_memset((unsigned char *)pwrctrlpriv, 0, sizeof(struct pwrctrl_priv));

	_free_pwrlock(&pwrctrlpriv->lock);

_func_exit_;
}


/*
Caller: xmit_thread

Check if the fw_pwrstate is okay for xmit.
If not (cpwm is less than P1 state), then the sub-routine
will raise the cpwm to be greater than or equal to P1. 

Calling Context: Passive

Return Value:

_SUCCESS: xmit_thread can write fifo/txcmd afterwards.
_FAIL: xmit_thread can not do anything.
*/
sint register_tx_alive(_adapter *padapter)
{
	uint res = _SUCCESS;
	
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(pwrctrl, XMIT_ALIVE);
	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("register_tx_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));

	if(pwrctrl->cpwm < PS_STATE_S2){
		set_rpwm(padapter, PS_STATE_S3);
		res = _FAIL;
	}

	_exit_pwrlock(&pwrctrl->lock);
	
_func_exit_;

#endif	/* CONFIG_PWRCTRL */

	return res;	

}

/*
Caller: cmd_thread

Check if the fw_pwrstate is okay for issuing cmd.
If not (cpwm should be is less than P2 state), then the sub-routine
will raise the cpwm to be greater than or equal to P2. 

Calling Context: Passive

Return Value:

_SUCCESS: cmd_thread can issue cmds to firmware afterwards.
_FAIL: cmd_thread can not do anything.
*/
sint register_cmd_alive(_adapter *padapter)
{
	uint res = _SUCCESS;
	
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(pwrctrl, CMD_ALIVE);
	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("register_cmd_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));

	if(pwrctrl->cpwm < PS_STATE_S2){
		set_rpwm(padapter, PS_STATE_S3);
		res = _FAIL;
	}

	_exit_pwrlock(&pwrctrl->lock);
_func_exit_;
#endif

	return res;
}


/*
Caller: rx_isr

Calling Context: Dispatch/ISR

Return Value:

*/
sint register_rx_alive(_adapter *padapter)
{

#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(pwrctrl, RECV_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("register_rx_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;
	
#endif /*CONFIG_PWRCTRL*/

	return _SUCCESS;
}


/*
Caller: evt_isr or evt_thread

Calling Context: Dispatch/ISR or Passive

Return Value:
*/
sint register_evt_alive(_adapter *padapter)
{

#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	register_task_alive(pwrctrl, EVT_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_info_,("register_evt_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;

#endif /*CONFIG_PWRCTRL*/

	return _SUCCESS;
}


/*
Caller: ISR

If ISR's txdone,
No more pkts for TX,
Then driver shall call this fun. to power down firmware again.
*/

void unregister_tx_alive(_adapter *padapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	unregister_task_alive(pwrctrl, XMIT_ALIVE);

	if((pwrctrl->cpwm > PS_STATE_S2) && (pwrctrl->pwr_mode > PS_MODE_ACTIVE)){
		if(pwrctrl->alives == 0){
			set_rpwm(padapter, PS_STATE_S0);
		}
	}

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("unregister_tx_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
	
	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}

/*
Caller: ISR

If ISR's txdone,
No more pkts for TX,
Then driver shall call this fun. to power down firmware again.
*/

void unregister_cmd_alive(_adapter *padapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	unregister_task_alive(pwrctrl, CMD_ALIVE);

	if((pwrctrl->cpwm > PS_STATE_S2) && (pwrctrl->pwr_mode > PS_MODE_ACTIVE)){
		if((pwrctrl->alives == 0)&&(check_fwstate(&padapter->mlmepriv, _FW_UNDER_LINKING)!=_TRUE)){
			set_rpwm(padapter, PS_STATE_S0);
		}
	}

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("unregister_cmd_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));

	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}


/*

Caller: ISR

*/
void unregister_rx_alive(_adapter *padapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	unregister_task_alive(pwrctrl, RECV_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("unregister_rx_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
	
	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;

#endif
}


void unregister_evt_alive(_adapter *padapter)
{
#ifdef CONFIG_PWRCTRL

	struct pwrctrl_priv *pwrctrl = &padapter->pwrctrlpriv;

_func_enter_;

	_enter_pwrlock(&pwrctrl->lock);

	unregister_task_alive(pwrctrl, EVT_ALIVE);

	RT_TRACE(_module_rtl871x_pwrctrl_c_,_drv_err_,("unregister_evt_alive: cpwm:%d alives:%x\n", pwrctrl->cpwm, pwrctrl->alives));
	
	_exit_pwrlock(&pwrctrl->lock);

_func_exit_;

#endif /*CONFIG_PWRCTRL*/
}



