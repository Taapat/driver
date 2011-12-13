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
*/

//=====================================================================
//
// rtuser:
//	1. User space application to demo how to use IOCTL function.
//	2. Most of the IOCTL function is defined as "CHAR" type and return with string message.
//	3. Use sscanf to get the raw data back from string message.
//	4. The command format "parameter=value" is same as iwpriv command format.
//	5. Remember to insert driver module and bring interface up prior execute rtuser.
//		change folder path to driver "Module"
//		dos2unix *		; in case the files are modified from other OS environment
//		chmod 644 *
//		chmod 755 Configure
//		make config
//		make
//		insmod RT273.ko
//		ifconfig mesh0 up
//
// Refer linux/if.h to have 
//		#define ifr_name	ifr_ifrn.ifrn_name			/* interface name 	*/
//
// Make:
//		cc -Wall -ortuser rtuser.c
//
// Run:
//		./rtuser OID
//
//=====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>				/* for close */
#include <linux/wireless.h>

//=============================================================================

#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE			0x8BE0
#endif
#define SIOCIWFIRSTPRIV			SIOCDEVPRIVATE
#endif

//
//SET/GET CONVENTION :
// * ------------------
// * Simplistic summary :
// *	o even numbered ioctls are SET, restricted to root, and should not
// *      return arguments (get_args = 0).
// *	o odd numbered ioctls are GET, authorised to anybody, and should
// *      not expect any arguments (set_args = 0).
//
#define RT_PRIV_IOCTL					(SIOCIWFIRSTPRIV + 0x0E)
#define RTPRIV_IOCTL_SET				(SIOCIWFIRSTPRIV + 0x02)
#define RTPRIV_IOCTL_BBP				(SIOCIWFIRSTPRIV + 0x03)
#define RTPRIV_IOCTL_MAC				(SIOCIWFIRSTPRIV + 0x05)
#define RTPRIV_IOCTL_E2P				(SIOCIWFIRSTPRIV + 0x11)
#define RTPRIV_IOCTL_STATISTICS			(SIOCIWFIRSTPRIV + 0x09)
#define RTPRIV_IOCTL_GSITESURVEY		(SIOCIWFIRSTPRIV + 0x0D)


#define OID_GET_SET_TOGGLE			0x8000

#define RT_QUERY_ATE_TXDONE_COUNT	0x0401
#define RT_QUERY_SIGNAL_CONTEXT		0x0402
#define RT_SET_APD_PID				(OID_GET_SET_TOGGLE + 0x0405)
#define RT_SET_DEL_MAC_ENTRY		(OID_GET_SET_TOGGLE + 0x0406)

//---------------------------------------------------------
// Mesh Extension
#define OID_802_11_MESH_SECURITY_INFO	0x0651
#define OID_802_11_MESH_ID				0x0652
#define OID_802_11_MESH_AUTO_LINK		0x0653
#define OID_802_11_MESH_LINK_STATUS		0x0654
#define OID_802_11_MESH_LIST			0x0655
#define OID_802_11_MESH_ROUTE_LIST		0x0656
#define OID_802_11_MESH_ADD_LINK		0x0657
#define OID_802_11_MESH_DEL_LINK		0x0658
#define OID_802_11_MESH_MAX_TX_RATE		0x0659
#define OID_802_11_MESH_CHANNEL			0x065A

#define RT_OID_802_11_MESH_SECURITY_INFO	(OID_GET_SET_TOGGLE + OID_802_11_MESH_SECURITY_INFO)
#define RT_OID_802_11_MESH_ID				(OID_GET_SET_TOGGLE + OID_802_11_MESH_ID)
#define RT_OID_802_11_MESH_AUTO_LINK		(OID_GET_SET_TOGGLE + OID_802_11_MESH_AUTO_LINK)
#define RT_OID_802_11_MESH_ADD_LINK			(OID_GET_SET_TOGGLE + OID_802_11_MESH_ADD_LINK)
#define RT_OID_802_11_MESH_DEL_LINK			(OID_GET_SET_TOGGLE + OID_802_11_MESH_DEL_LINK)
#define RT_OID_802_11_MESH_MAX_TX_RATE		(OID_GET_SET_TOGGLE + OID_802_11_MESH_MAX_TX_RATE)
#define RT_OID_802_11_MESH_CHANNEL			(OID_GET_SET_TOGGLE + OID_802_11_MESH_CHANNEL)

//---------------------------------------------------------

#ifndef 	TRUE
#define 	TRUE				1
#endif

#ifndef 	FALSE
#define 	FALSE				0
#endif

#define MAC_ADDR_LEN			6
#define ETH_LENGTH_OF_ADDRESS	6
#define MAX_LEN_OF_MAC_TABLE	64

#define MAX_MESH_ID_LEN			32
#define MAX_NEIGHBOR_NUM		64
#define MAX_MESH_LINK_NUM		4
#define MAX_HOST_NAME_LEN		32

#define PACKED  __attribute__ ((packed))
//---------------------------------------------------------

// Mesh definition
#define MESH_MAX_LEN_OF_FORWARD_TABLE	48

typedef struct _RT_MESH_ROUTE_ENTRY
{
	unsigned char       MeshDA[MAC_ADDR_LEN];
	unsigned long       Dsn;
	unsigned char       NextHop[MAC_ADDR_LEN];
	unsigned long       Metric;
} RT_MESH_ROUTE_ENTRY, *PRT_MESH_ROUTE_ENTRY;

typedef struct _RT_MESH_ROUTE_TABLE
{
	unsigned char           Num;
	RT_MESH_ROUTE_ENTRY     Entry[MESH_MAX_LEN_OF_FORWARD_TABLE];
} RT_MESH_ROUTE_TABLE, *PRT_MESH_ROUTE_TABLE;

typedef struct PACKED _MESH_SECURITY_INFO
{
	unsigned long   Length;				// Length of this structure    	  	
	unsigned char   EncrypType;			// indicate Encryption Type
	unsigned char   KeyIndex;			// Default Key Index
	unsigned char   KeyLength;			// length of key in bytes
	unsigned char   KeyMaterial[64];	// the key Material
} MESH_SECURITY_INFO, *PMESH_SECURITY_INFO;

typedef struct _MESH_NEIGHBOR_ENTRY_INFO
{
	char Rssi;
	unsigned char HostName[MAX_HOST_NAME_LEN];
	unsigned char MacAddr[MAC_ADDR_LEN];
	unsigned char MeshId[MAX_MESH_ID_LEN];
	unsigned char Channel;
	unsigned char Status; 				//0:idle, 1:connected.
	unsigned char MeshEncrypType;
} MESH_NEIGHBOR_ENTRY_INFO, *PMESH_NEIGHBOR_ENTRY_INFO;

typedef struct _MESH_NEIGHBOR_INFO
{
	unsigned char				num;
	MESH_NEIGHBOR_ENTRY_INFO	Entry[MAX_NEIGHBOR_NUM];
} MESH_NEIGHBOR_INFO, *PMESH_NEIGHBOR_INFO;

typedef struct _MESH_LINK_ENTRY_INFO
{
	unsigned char Status;
	char Rssi;
	unsigned char CurTxRate;
	unsigned char PeerMacAddr[MAC_ADDR_LEN];
} MESH_LINK_ENTRY_INFO, *PMESH_LINK_ENTRY_INFO;

typedef struct _MESH_LINK_INFO
{
	MESH_LINK_ENTRY_INFO Entry[MAX_MESH_LINK_NUM];
} MESH_LINK_INFO, *PMESH_LINK_INFO;

//---------------------------------------------------------


//=============================================================================

int main( int argc, char ** argv )
{
#define DATA_BUFFER_SIZE	8192
	char        name[25];
	int     socket_id;
	struct  iwreq wrq;
	int     ret = 1, cmd = 0;
	char        *base;//, *base1;
	char        *data;

	// open socket based on address family: AF_NET ----------------------------
	socket_id = socket(AF_INET, SOCK_DGRAM, 0);
	if(socket_id < 0)
	{
		printf("\nrtuser::error::Open socket error!\n\n");
		return -1;
	}

	data = malloc(DATA_BUFFER_SIZE);
	if (data == NULL)
	{
		printf("unable to alloc data buffer, size (%d)\n", DATA_BUFFER_SIZE);
		return -1;
	}

	// set interface name as "mesh0" --------------------------------------------
	sprintf(name, "mesh0");
	memset(data, 0, DATA_BUFFER_SIZE);
	//
	//example of ioctl function ==========================================
	//

	base = argv[1];
	//base1 = argv[2];

	if(argc != 2)
		goto rtuser_exit;

	if(strstr(base, "OID_802_11_MESH_ROUTE_LIST"))
		cmd = OID_802_11_MESH_ROUTE_LIST;
	else if(strstr(base, "RT_OID_802_11_MESH_SECURITY_INFO"))
		cmd = RT_OID_802_11_MESH_SECURITY_INFO;
	else if(strstr(base, "OID_802_11_MESH_SECURITY_INFO"))
		cmd = OID_802_11_MESH_SECURITY_INFO;
	else if(strstr(base, "RT_OID_802_11_MESH_ID"))
		cmd = RT_OID_802_11_MESH_ID;
	else if(strstr(base, "OID_802_11_MESH_ID"))
		cmd = OID_802_11_MESH_ID;
	else if(strstr(base, "RT_OID_802_11_MESH_AUTO_LINK"))
		cmd = RT_OID_802_11_MESH_AUTO_LINK;
	else if(strstr(base, "OID_802_11_MESH_AUTO_LINK"))
		cmd = OID_802_11_MESH_AUTO_LINK;
	else if(strstr(base, "RT_OID_802_11_MESH_ADD_LINK"))
		cmd = RT_OID_802_11_MESH_ADD_LINK;
	else if(strstr(base, "RT_OID_802_11_MESH_DEL_LINK"))
		cmd = RT_OID_802_11_MESH_DEL_LINK;
	else if(strstr(base, "OID_802_11_MESH_LINK_STATUS"))
		cmd = OID_802_11_MESH_LINK_STATUS;
	else if(strstr(base, "OID_802_11_MESH_LIST"))
		cmd = OID_802_11_MESH_LIST;
	else if (strstr(base, "RT_OID_802_11_MESH_CHANNEL"))
		cmd = RT_OID_802_11_MESH_CHANNEL;
	else if (strstr(base, "OID_802_11_MESH_CHANNEL"))
		cmd = OID_802_11_MESH_CHANNEL;
	else if (strstr(base, "RT_OID_802_11_MESH_MAX_TX_RATE"))
		cmd = RT_OID_802_11_MESH_MAX_TX_RATE;
	else if (strstr(base, "OID_802_11_MESH_MAX_TX_RATE"))
		cmd = OID_802_11_MESH_MAX_TX_RATE;

	switch(cmd)
	{
		case OID_802_11_MESH_ROUTE_LIST:
		{
			RT_MESH_ROUTE_TABLE     *sp;
			int i;

			//Get Mesh Route List
			printf("\nrtuser::get OID_802_11_MESH_ROUTE_LIST \n\n");
			memset(data, 0, sizeof(RT_MESH_ROUTE_TABLE));
			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(RT_MESH_ROUTE_TABLE);
			wrq.u.data.pointer = data;
			wrq.u.data.flags = OID_802_11_MESH_ROUTE_LIST;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get OID_802_11_MESH_ROUTE_LIST\n\n");
				goto rtuser_exit;
			}

			sp = (RT_MESH_ROUTE_TABLE *)wrq.u.data.pointer;
			printf("\n===== MESH_ROUTE_TABLE =====\n\n");
			for(i = 0; i < sp->Num; i++)
			{
				printf("Dest Mesh Addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
					sp->Entry[i].MeshDA[0], sp->Entry[i].MeshDA[1], 
					sp->Entry[i].MeshDA[2], sp->Entry[i].MeshDA[3], 
					sp->Entry[i].MeshDA[4], sp->Entry[i].MeshDA[5]);
				printf("Dsn       = %ld\n", sp->Entry[i].Dsn);
				printf("NextHop Addr = %02x:%02x:%02x:%02x:%02x:%02x\n", 
					sp->Entry[i].NextHop[0], sp->Entry[i].NextHop[1], 
					sp->Entry[i].NextHop[2], sp->Entry[i].NextHop[3], 
					sp->Entry[i].NextHop[4], sp->Entry[i].NextHop[5]);
				printf("Metric   = %ld\n\n", sp->Entry[i].Metric);
			}
		}
		break;

		case OID_802_11_MESH_SECURITY_INFO:
		{
			PMESH_SECURITY_INFO         pMeshSecurity;

				// Get Mesh Security
			printf("\nrtuser::OID_802_11_MESH_SECURITY_INFO \n\n");
			memset(data, 0, sizeof(MESH_SECURITY_INFO));

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(MESH_SECURITY_INFO);
			wrq.u.data.pointer = data;
			wrq.u.data.flags = OID_802_11_MESH_SECURITY_INFO;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get mesh security\n\n");
				goto rtuser_exit;
			}

			pMeshSecurity = (PMESH_SECURITY_INFO)wrq.u.data.pointer;

				// Show Encryption Type
			switch(pMeshSecurity->EncrypType)
			{
				case 0:             
					printf("Mesh Encryption is OPEN-NONE\n\n");
				break;

				case 1:
					printf("Mesh Encryption is OPEN-WEP\n\n");												
				break;

				case 2:
					printf("Mesh Encryption is WPANONE-TKIP\n\n");								
				break;

				case 3:
					printf("Mesh Encryption is WPANONE-AES\n\n");
				break;

				default:
					printf("Unsupported Mesh Encryption Type\n\n");
				break;
			}

			// Show key, key length and key index
			if (pMeshSecurity->KeyLength > 0)
			{				
				printf("Key=%s, length=%d\n", pMeshSecurity->KeyMaterial, pMeshSecurity->KeyLength);					
				printf("The key Index = %d \n", pMeshSecurity->KeyIndex);
			}
										
		}
		break;

		case RT_OID_802_11_MESH_SECURITY_INFO:
		{
			MESH_SECURITY_INFO  MeshSecurity;
			// Please modify the below parameter to set security			
			// EncrypType, KeyIndex	and MeshKey		
			unsigned char	EncrypType 	= 0;	// 0:OPEN-NONE, 1:OPEN-WEP, 2:WPANONE-TKIP, 3:WPANONE-AES
			unsigned char	KeyIndex	= 1;	// Default key index
			const char *MeshKey = "12345678";	// Key material			
			
			// Set Mesh Security
			printf("\nrtuser::set mesh security \n\n");
			memset(&MeshSecurity, 0, sizeof(MESH_SECURITY_INFO));
			
			// OPEN-WEP mode
			if (EncrypType == 1)
			{
				if ((strlen(MeshKey) == 5) || (strlen(MeshKey) == 13) || 
					(strlen(MeshKey) == 10) || (strlen(MeshKey) == 26))
				{
					printf("set security as OPEN-WEP\n\n");
					MeshSecurity.KeyLength = strlen(MeshKey);
					memcpy(MeshSecurity.KeyMaterial, MeshKey, MeshSecurity.KeyLength);
				}	
				else
				{
					printf("set mesh security::invalid WEP key and set security as OPEN-NONE\n\n");
					EncrypType = 0;
				}					
			}
			else if ((EncrypType == 2) || (EncrypType == 3))
			{
				// The WPA key must be 8~64 characters
				if ((strlen(MeshKey) >= 8) && (strlen(MeshKey) <= 64))
				{					
					printf("set security as %s\n\n", (EncrypType == 2) ? "WPANONE-TKIP" : "WPANONE-AES");						
					MeshSecurity.KeyLength = strlen(MeshKey);
					memcpy(MeshSecurity.KeyMaterial, MeshKey, MeshSecurity.KeyLength);
				}	
				else
				{
					printf("set mesh security::invalid WPA key and set security as OPEN-NONE\n\n");
					EncrypType = 0;
				}					
			}										
			else
			{
				printf("set security as OPEN-NONE\n\n");
				EncrypType = 0;
			}	
				
			// Set Mesh Encryption Type 
			MeshSecurity.EncrypType = EncrypType;	
			 
			// Set Default Key Index
			if (KeyIndex >= 1 &&  KeyIndex <= 4)
				MeshSecurity.KeyIndex = KeyIndex;	// valid value : 1~4
			else
				MeshSecurity.KeyIndex = 1;
			
			// In WPANONE mode, the default key shall be 1.
			if ((EncrypType == 2) || (EncrypType == 3))
				MeshSecurity.KeyIndex = 1;	
						
			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(MESH_SECURITY_INFO);
			wrq.u.data.pointer = (char *)&MeshSecurity;
			wrq.u.data.flags = RT_OID_802_11_MESH_SECURITY_INFO;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh security\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case OID_802_11_MESH_ID:
		{
				// Get Mesh ID
			printf("\nrtuser::set OID_802_11_MESH_ID \n\n");

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = MAX_MESH_ID_LEN;
			wrq.u.data.pointer = data;
			wrq.u.data.flags = OID_802_11_MESH_ID;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get mesh id\n\n");
				goto rtuser_exit;
			}

			printf("Mesh-Id=%s\n", (unsigned char *)wrq.u.data.pointer);
		}
		break;

	case RT_OID_802_11_MESH_ID:
		{
			unsigned char MeshId[MAX_MESH_ID_LEN];

				// Set Mesh ID
			printf("\nrtuser::set RT_OID_802_11_MESH_ID \n\n");
			memset(MeshId, 0, sizeof(MAX_MESH_ID_LEN));
			memcpy(MeshId, "SampleMesh", strlen("SampleMesh") + 1);

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = strlen("SampleMesh");
			wrq.u.data.pointer = (char *)&MeshId;
			wrq.u.data.flags = RT_OID_802_11_MESH_ID;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh Id\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case OID_802_11_MESH_AUTO_LINK:
		{
			unsigned char MeshAutoLink;
			// Get Mesh Auto Link Setting
			printf("\nrtuser::set OID_802_11_MESH_AUTO_LINK \n\n");
			

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = 1;
			wrq.u.data.pointer = (char *)&MeshAutoLink;
			wrq.u.data.flags = OID_802_11_MESH_AUTO_LINK;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get mesh auto link setting\n\n");
				goto rtuser_exit;
			}

			printf("Mesh-Auto Link=%s\n", MeshAutoLink == 1 ? "Enable" : "Disable");
		}
		break;

	case RT_OID_802_11_MESH_AUTO_LINK:
		{
			unsigned char MeshAutoLink;

				// Set Mesh Auto Link
			printf("\nrtuser::set RT_OID_802_11_MESH_AUTO_LINK \n\n");
			MeshAutoLink = 0;

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(unsigned char);
			wrq.u.data.pointer = (char *)&MeshAutoLink;
			wrq.u.data.flags = RT_OID_802_11_MESH_AUTO_LINK;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh auto link\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case RT_OID_802_11_MESH_ADD_LINK:
		{
			unsigned char PeerMac[MAC_ADDR_LEN] = {0x0, 0xc, 0x43, 0x28, 0x61, 0x10};

				// Set Peer Link MAC Addr.
			printf("\nrtuser::set RT_OID_802_11_MESH_ADD_LINK \n\n");

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(PeerMac);
			wrq.u.data.pointer = (char *)&PeerMac;
			wrq.u.data.flags = RT_OID_802_11_MESH_ADD_LINK;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh add link\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case RT_OID_802_11_MESH_DEL_LINK:
		{
			unsigned char PeerMac[MAC_ADDR_LEN] = {0x0, 0xc, 0x43, 0x28, 0x61, 0x10};

				// Set Peer Link MAC Addr.
			printf("\nrtuser::set RT_OID_802_11_MESH_DEL_LINK \n\n");

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(PeerMac);
			wrq.u.data.pointer = (char *)&PeerMac;
			wrq.u.data.flags = RT_OID_802_11_MESH_DEL_LINK;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh del link\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case OID_802_11_MESH_LINK_STATUS:
		{
			PMESH_LINK_INFO			pMeshLinkInfo;
			PMESH_LINK_ENTRY_INFO	pMeshLinkEntry;
			int ii;

				// Get Mesh Link Info
			printf("\nrtuser::set OID_802_11_MESH_LINK_STATUS \n\n");
			memset(data, 0, sizeof(MESH_LINK_INFO));

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(MESH_LINK_INFO);
			wrq.u.data.pointer = data;
			wrq.u.data.flags = OID_802_11_MESH_LINK_STATUS;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get mesh link info\n\n");
				goto rtuser_exit;
			}

			pMeshLinkInfo = (PMESH_LINK_INFO)wrq.u.data.pointer;

			for(ii = 0; ii < MAX_MESH_LINK_NUM; ii++)
			{
				pMeshLinkEntry = (PMESH_LINK_ENTRY_INFO)&pMeshLinkInfo->Entry[ii];
				printf(" PeerMac = %02x:%02x:%02x:%02x:%02x:%02x:",
					pMeshLinkEntry->PeerMacAddr[0], pMeshLinkEntry->PeerMacAddr[1],
					pMeshLinkEntry->PeerMacAddr[2], pMeshLinkEntry->PeerMacAddr[3],
					pMeshLinkEntry->PeerMacAddr[4], pMeshLinkEntry->PeerMacAddr[5]);
				printf(" Status =%d\n", pMeshLinkEntry->Status);	//0:idle
														//1:connected.
			}
		}
		break;

	case OID_802_11_MESH_LIST:
		{
			PMESH_NEIGHBOR_INFO			pMeshNeighInfo;
			PMESH_NEIGHBOR_ENTRY_INFO	pMeshNeighEntry;
			int ii;

				// Get Mesh Neighbor Info
			printf("\nrtuser::set OID_802_11_MESH_LIST \n\n");
			memset(data, 0, sizeof(MESH_NEIGHBOR_INFO));

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(MESH_NEIGHBOR_INFO);
			wrq.u.data.pointer = data;
			wrq.u.data.flags = OID_802_11_MESH_LIST;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::get mesh neighbor info\n\n");
				goto rtuser_exit;
			}

			pMeshNeighInfo = (PMESH_NEIGHBOR_INFO)wrq.u.data.pointer;

			for(ii = 0; ii < pMeshNeighInfo->num; ii++)
			{
				pMeshNeighEntry = (PMESH_NEIGHBOR_ENTRY_INFO)&pMeshNeighInfo->Entry[ii];

				printf("Rssi =%d", pMeshNeighEntry->Rssi);
				printf(" PeerMac = %02x:%02x:%02x:%02x:%02x:%02x:",
					pMeshNeighEntry->MacAddr[0], pMeshNeighEntry->MacAddr[1],
					pMeshNeighEntry->MacAddr[2], pMeshNeighEntry->MacAddr[3],
					pMeshNeighEntry->MacAddr[4], 
					pMeshNeighEntry->MacAddr[5]);
				printf(" MeshId = %s", pMeshNeighEntry->MeshId);
				printf(" Ch = %d", pMeshNeighEntry->Channel);
				printf(" Status =%d", pMeshNeighEntry->Status);	// 0:idle, 1:connected.

				switch(pMeshNeighEntry->MeshEncrypType)
				{
					case 0:             
						printf(" Encryption = OPEN-NONE\n");
					break;

					case 1:
						printf(" Encryption = OPEN-WEP\n");												
					break;

					case 2:
						printf(" Encryption = WPANONE-TKIP\n");								
					break;

					case 3:
						printf(" Encryption = WPANONE-AES\n");
					break;

					default:
						printf(" Encryption = OPEN-NONE\n");
					break;
				}
			}
		}
		break;

	case RT_OID_802_11_MESH_MAX_TX_RATE:
		{
			unsigned char MaxTxRate;

				// Set Mesh Auto Link
			printf("\nrtuser::set RT_OID_802_11_MESH_MAX_TX_RATE \n\n");
			MaxTxRate = 11;

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(unsigned char);
			wrq.u.data.pointer = (char *)& MaxTxRate;
			wrq.u.data.flags = RT_OID_802_11_MESH_MAX_TX_RATE;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh max tx rate.\n\n");
				goto rtuser_exit;
			}
		}
		break;

	case OID_802_11_MESH_MAX_TX_RATE:
		{
			unsigned char MaxTxRate;

				// Set Mesh Auto Link
			printf("\nrtuser::set OID_802_11_MESH_MAX_TX_RATE \n\n");

			strcpy(wrq.ifr_name, name);
			wrq.u.data.length = sizeof(unsigned char);
			wrq.u.data.pointer = (char *)& MaxTxRate;
			wrq.u.data.flags = OID_802_11_MESH_MAX_TX_RATE;
			ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

			if(ret != 0)
			{
				printf("\nrtuser::error::set mesh max tx rate.\n\n");
				goto rtuser_exit;
			}
			printf("Current Mesh Tx rate=%d\n", MaxTxRate);
		}
		break;

	case RT_OID_802_11_MESH_CHANNEL:
			{
				unsigned char Channel;

				// Set Mesh Channel
				printf("\nrtuser::set RT_OID_802_11_MESH_CHANNEL \n\n");
				Channel = 11;

				strcpy(wrq.ifr_name, name);
				wrq.u.data.length = sizeof(unsigned char);
				wrq.u.data.pointer = (char *)& Channel;
				wrq.u.data.flags = RT_OID_802_11_MESH_CHANNEL;
				ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

				if(ret != 0)
				{
					printf("\nrtuser::error::set mesh channel.\n\n");
					goto rtuser_exit;
				}
			}
			break;

		case OID_802_11_MESH_CHANNEL:
			{
				unsigned char Channel;

				// Get Mesh Channel
				printf("\nrtuser::set OID_802_11_MESH_CHANNEL \n\n");

				strcpy(wrq.ifr_name, name);
				wrq.u.data.length = sizeof(unsigned char);
				wrq.u.data.pointer = (char *)& Channel;
				wrq.u.data.flags = OID_802_11_MESH_CHANNEL;
				ret = ioctl(socket_id, RT_PRIV_IOCTL, &wrq);

				if(ret != 0)
				{
					printf("\nrtuser::error::get mesh channel.\n\n");
					goto rtuser_exit;
				}
				printf("Current Mesh Channel=%d\n", Channel);
			}
			break;

	default:
		printf("\nUnsupported OID type\n");
		break;
	}

	rtuser_exit:
	if(socket_id >= 0)
		close(socket_id);

	if (data != NULL)
		free(data);

	if(ret)
		return ret;
	else
		return 0;
}// endof main()
