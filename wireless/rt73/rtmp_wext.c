/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 5F, No. 36 Taiyuan St.
 * Jhubei City
 * Hsinchu County 302, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2008, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  * 
 * it under the terms of the GNU General Public License as published by  * 
 * the Free Software Foundation; either version 2 of the License, or     * 
 * (at your option) any later version.                                   * 
 *                                                                       * 
 * This program is distributed in the hope that it will be useful,       * 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        * 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * 
 * GNU General Public License for more details.                          * 
 *                                                                       * 
 * You should have received a copy of the GNU General Public License     * 
 * along with this program; if not, write to the                         * 
 * Free Software Foundation, Inc.,                                       * 
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             * 
 *                                                                       * 
 *************************************************************************
 
    Module Name:
    rtmp_wext.h

    Abstract:
    This file was created for wpa_supplicant general wext driver support.

    Revision History:
    Who         When          What
    --------    ----------    ----------------------------------------------
    Shiang      2007/04/03    Initial
    
*/

#include "rtmp_wext.h"


#ifdef NATIVE_WPA_SUPPLICANT_SUPPORT

int wext_notify_event_assoc(
	RTMP_ADAPTER *pAd, 
	USHORT iweCmd, 
	int assoc)
{
#define WEXT_ASSOCINFO_PREFIX "ASSOCINFO(ReqIEs="
#define WEXT_ASSOCINFO_MIDFIX " RespIEs="
#define WEXT_ASSOCINFO_POSTFIX ")"

	union iwreq_data wrqu;
	unsigned char *pAssocInfo, *p, *ies;
	int i, infoLen;
		
	if (assoc)
	{
		printk("ReqVarIELen=%d! RespIELen=%d!\n", pAd->PortCfg.ReqVarIELen, pAd->PortCfg.ResVarIELen);
		infoLen = sizeof(WEXT_ASSOCINFO_PREFIX) + pAd->PortCfg.ReqVarIELen * 2 + 
					sizeof(WEXT_ASSOCINFO_MIDFIX) + pAd->PortCfg.ResVarIELen * 2 + 
					sizeof(WEXT_ASSOCINFO_POSTFIX) + 1;
		printk("infoLen=%d!\n", infoLen);

		if (infoLen > IW_CUSTOM_MAX)
		{
			DBGPRINT(RT_DEBUG_TRACE, "%s():assocInfo too large to send to user space! (len=%d)\n", __FUNCTION__, infoLen);
			return -1;
		}
	
		pAssocInfo = kmalloc(infoLen, MEM_ALLOC_FLAG);
		if (!pAssocInfo) 
		{
		    DBGPRINT(RT_DEBUG_TRACE, "%s():allocate memory for assocInfo failed!\n", __FUNCTION__);
			return -1;
		}
		NdisZeroMemory(pAssocInfo, infoLen);
		/*
		 * TODO: backwards compatibility would require that IWEVCUSTOM
		 * is sent even if WIRELESS_EXT > 17. This version does not do
		 * this in order to allow wpa_supplicant to be tested with
		 * WE-18.
		 */
		//Prepare Prefix
		p = pAssocInfo;
		p += sprintf(p, WEXT_ASSOCINFO_PREFIX);

		//Prepare ReqIEs
		ies = &pAd->PortCfg.ReqVarIEs[0];
		for (i = 0; i < pAd->PortCfg.ReqVarIELen; i++)
			p += sprintf(p, "%02x", ies[i]);

		//Prepare Midfix
		p += sprintf(p, WEXT_ASSOCINFO_MIDFIX);

		//Prepare RespIEs
		ies = &pAd->PortCfg.ResVarIEs[0];
		for (i = 0; i < pAd->PortCfg.ResVarIELen; i++)
			p += sprintf(p, "%02x", ies[i]);

		//Prepare Postfix
		p += sprintf(p, WEXT_ASSOCINFO_POSTFIX);
		
		wrqu.data.length = infoLen;
		DBGPRINT(RT_DEBUG_TRACE, "adding %d bytes\n", wrqu.data.length);
		printk("%s():AssocInfo=\n\t%s!\n", __FUNCTION__, pAssocInfo);
		wireless_send_event(pAd->net_dev, IWEVCUSTOM, &wrqu, pAssocInfo);
		
		kfree(pAssocInfo);
	}


	//Send iweCmd to wpa_supplicant.
	NdisZeroMemory(&wrqu, sizeof(wrqu));
	wrqu.ap_addr.sa_family = ARPHRD_ETHER;
	if (assoc)
		NdisMoveMemory(wrqu.ap_addr.sa_data, pAd->MlmeAux.Bssid, MAC_ADDR_LEN);
	else
		NdisZeroMemory(wrqu.ap_addr.sa_data, MAC_ADDR_LEN);
	
	wireless_send_event(pAd->net_dev, iweCmd, &wrqu, NULL);

	return 0;
	
}


int wext2Rtmp_Security_Wrapper(RTMP_ADAPTER *pAd)
{

#if WIRELESS_EXT > 17
	int authMode = Ndis802_11EncryptionDisabled;
	int wepStatus;
	// 1:IW_AUTH_WPA_VERSION_DISABLED, if wpa_ie_length = 0
	// 2:IW_AUTH_WPA_VERSION_WPA, if 
	// 3:IW_AUTH_WPA_VERSION_WPA2, if wpa_ie[0]= 0x30 = RSN_IE_OID

	printk("Before wrapper, AuthMode=%d, wepStatus=%d, pCipher=%d, gCipher=%d, IEEE8021X=%d\n", 
			pAd->PortCfg.AuthMode, pAd->PortCfg.WepStatus, pAd->PortCfg.PairCipher, 
			pAd->PortCfg.GroupCipher, pAd->PortCfg.IEEE8021X);
	
	pAd->PortCfg.IEEE8021X = FALSE;
	
	if (pAd->PortCfg.RSN_IELen == 0 || pAd->PortCfg.wx_wpa_version == IW_AUTH_WPA_VERSION_DISABLED)
	{
		if (pAd->PortCfg.wx_auth_alg & IW_AUTH_ALG_SHARED_KEY)
		{
			if (pAd->PortCfg.wx_auth_alg & IW_AUTH_ALG_OPEN_SYSTEM)
				authMode = Ndis802_11AuthModeAutoSwitch;
			else
				authMode = Ndis802_11AuthModeShared;
		}
		else
		{
			authMode = Ndis802_11AuthModeOpen;
		}
	}
	else if((pAd->PortCfg.RSN_IE[0] == WPA2RSNIE) || 
			(pAd->PortCfg.wx_wpa_version == IW_AUTH_WPA_VERSION_WPA2))
	{
		if (pAd->PortCfg.wx_key_mgmt == IW_AUTH_KEY_MGMT_PSK)
			authMode = Ndis802_11AuthModeWPA2PSK;
		else
			authMode = Ndis802_11AuthModeWPA2;
		
	}
	else if((pAd->PortCfg.RSN_IE[0] == WPARSNIE) || 
			(pAd->PortCfg.wx_wpa_version == IW_AUTH_WPA_VERSION_WPA))
	{
		if (pAd->PortCfg.wx_key_mgmt == IW_AUTH_KEY_MGMT_PSK)
			authMode = Ndis802_11AuthModeWPAPSK;
		else
			authMode = Ndis802_11AuthModeWPA;
	}
	else
	{
		//(wx_auth_alg == IW_AUTH_ALG_LEAP) //Wpa-supplicant may set the Auth mode as this, but we didn't support it.
		if (pAd->PortCfg.wx_key_mgmt == IW_AUTH_KEY_MGMT_802_1X)
			pAd->PortCfg.IEEE8021X = TRUE;
		else
			return -EINVAL;
	}
	
	// wepStatus setting
	switch(pAd->PortCfg.wx_pairwise)
	{
		case IW_AUTH_CIPHER_NONE:
			if (pAd->PortCfg.wx_groupCipher == IW_AUTH_CIPHER_CCMP)
				wepStatus = Ndis802_11Encryption3Enabled;
			else if (pAd->PortCfg.wx_groupCipher == IW_AUTH_CIPHER_TKIP)
				wepStatus = Ndis802_11Encryption2Enabled;
	        else
				wepStatus = Ndis802_11EncryptionDisabled;
			break;
		case IW_AUTH_CIPHER_WEP40:
		case IW_AUTH_CIPHER_WEP104:
			wepStatus = Ndis802_11Encryption1Enabled;
			break;
		case IW_AUTH_CIPHER_TKIP:
			wepStatus = Ndis802_11Encryption2Enabled;
			break;
		case IW_AUTH_CIPHER_CCMP:
			wepStatus = Ndis802_11Encryption3Enabled;
			break;
		default:
			wepStatus = Ndis802_11EncryptionDisabled;
			break;
	}
					
	if (authMode != pAd->PortCfg.AuthMode || pAd->PortCfg.WepStatus != wepStatus)
		pAd->bConfigChanged = TRUE;

	pAd->PortCfg.AuthMode = authMode;
	pAd->PortCfg.WepStatus = wepStatus;
	pAd->PortCfg.OrigWepStatus = wepStatus;
	pAd->PortCfg.PairCipher = wepStatus;
	pAd->PortCfg.GroupCipher = wepStatus;

	printk("After wrapper, the AuthMode=%d! wepStatus=%d!\n", pAd->PortCfg.AuthMode, pAd->PortCfg.WepStatus);
	
#endif

	return 0;
}


#endif // NATIVE_WPA_SUPPLICANT_SUPPORT

