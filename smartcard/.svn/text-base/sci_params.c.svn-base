/*
 * 	Copyright (C) 2010 Duolabs Srl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "sci.h"
#include "sci_types.h"

#define MAIN_FREQ			100000000
#define MAIN_FREQ_P1		1000
#define MAIN_FREQ_P2		100000
#define	PW_2_16				65536

#define	THRESHOLD_BAUDRATE	19200
#define	CLOCK_3572			3572000
#define	CLOCK_417			4166667
#define	CLOCK_500			5000000
#define	CLOCK_625			6250000

extern SCI_CONTROL_BLOCK    sci_cb[SCI_NUMBER_OF_CONTROLLERS];
extern ULONG                sci_driver_init;
extern INT                  debug;
extern void _set_reg (ULONG sci_id, BASE_ADDR base_address, ULONG reg, UINT bits, UINT mask);
extern ULONG _get_reg(ULONG sci_id, BASE_ADDR base_address, ULONG reg);
extern void _set_reg_writeonly(ULONG sci_id, BASE_ADDR base_address, ULONG reg, UINT bits);
extern INT smartcard_voltage_config(ULONG sci_id, UINT vcc);


extern void smartcard_reset(ULONG sci_id, unsigned char wait);

/* Local function calls for set parameters */
SCI_ERROR sci_osi_set_para_T (ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);
SCI_ERROR sci_osi_set_para_f (ULONG sci_id, SCI_PARAMETERS *p_sci_parameters);
SCI_ERROR sci_osi_set_para_ETU (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_WWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_CWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_BWT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_EGT (ULONG sci_id,
                                SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_CLK_p (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);

SCI_ERROR sci_osi_set_para_check (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);
SCI_ERROR sci_osi_set_para_class (ULONG sci_id,
                                  SCI_PARAMETERS *p_sci_parameters
);

/*****************************************************************************
** Function:    sci_atom_set_para_f
**
** Purpose:     Set the current Smart Card parameters of frequency.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
*****************************************************************************/
void sci_atom_set_para_f(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    ULONG divisor, frequency;

    /* set scclk to nearest possible frequency */
    divisor = 0;
    do
    {
        divisor++;
        frequency = __STB_SYS_CLK / (2 * divisor);
    }
    while(frequency > p_sci_parameters->f);

    sci_cb[sci_id].sci_parameters.f = frequency;
}

/**
***************************************************************************
* @brief  Set the current Smart Card parameters.
* @param  sci_id zero-based number to identify smart card controller
* @param  p_sci_parameters input pointer to Smart Card parameters
* @return SCI_ERROR_OK: if successful
*         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
*             p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("\n");
    PDEBUG("'sci_osi_set_para':\n");
    PDEBUG("p_sci_parameters->T:%d\n",p_sci_parameters->T);
    PDEBUG("p_sci_parameters->f:%d\n",p_sci_parameters->f);
    PDEBUG("p_sci_parameters->ETU:%d\n",p_sci_parameters->ETU);
    PDEBUG("p_sci_parameters->WWT:%d\n",p_sci_parameters->WWT);
    PDEBUG("p_sci_parameters->CWT:%d\n",p_sci_parameters->CWT);
    PDEBUG("p_sci_parameters->BWT:%d\n",p_sci_parameters->BWT);
    PDEBUG("p_sci_parameters->EGT:%d\n",p_sci_parameters->EGT);
    PDEBUG("p_sci_parameters->P:%d\n",p_sci_parameters->P);
    PDEBUG("p_sci_parameters->I:%d\n",p_sci_parameters->I);
    PDEBUG("p_sci_parameters->U:%d\n",p_sci_parameters->U);
    PDEBUG("\n");

	if(sci_cb[sci_id].atr_status!=SCI_ATR_READY)
	{
		printk("ATR wasn't read\n");
		return (rc);
	}

    if((p_sci_parameters != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
    {
        rc = sci_osi_set_para_T    (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_f    (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_ETU  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_WWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_CWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_BWT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_EGT  (sci_id, p_sci_parameters);
        rc = sci_osi_set_para_CLK_p(sci_id, p_sci_parameters);
        rc = sci_osi_set_para_check(sci_id, p_sci_parameters);
        rc = sci_osi_set_para_class(sci_id, p_sci_parameters);
        /* not supported, just set the values */
        sci_cb[sci_id].sci_parameters.P = p_sci_parameters->P;
        sci_cb[sci_id].sci_parameters.I = p_sci_parameters->I;
    }
    else
    {
        rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/**
***************************************************************************
* @brief  Set the current Smart Card parameters. This function calls
*         set_parameters(), which is also called internally.
* @param  sci_id zero-based number to identify smart card controller
* @param  p_sci_parameters input pointer to Smart Card parameters
* @return SCI_ERROR_OK: if successful
*         SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
*             sci_init() has been made
*         SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
*             p_sci_parameters is zero.
*         SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
***************************************************************************/
SCI_ERROR sci_osi_set_parameters(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc=SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);

    if(sci_driver_init == 1)
    {
        if((p_sci_parameters != 0) && (sci_id < SCI_NUMBER_OF_CONTROLLERS))
        {
            if(sci_osi_is_card_activated(sci_id) == SCI_CARD_PRESENT)
            {
                PDEBUG("Before sci_osi_set_para()\n");
                rc = sci_osi_set_para(sci_id, p_sci_parameters);
            }
            else
            {
                rc = SCI_ERROR_CARD_NOT_ACTIVATED;
            }
        }
        else
        {
            rc = SCI_ERROR_PARAMETER_OUT_OF_RANGE;
        }
    }
    else
    {
        rc = SCI_ERROR_DRIVER_NOT_INITIALIZED;
    }

    if(rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
    }
    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}

/****************************************************************************
** Function:    sci_osi_get_parameters
**
** Purpose:     Retrieve the current Smart Card parameters.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: output pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_DRIVER_NOT_INITIALIZED: if no successful call to
**                  sci_init() has been made
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
**              SCI_ERROR_CARD_NOT_ACTIVATED: if card is not activated
*****************************************************************************/
SCI_ERROR sci_osi_get_parameters(ULONG sci_id, 
                                 SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    PDEBUG("card[%d] enter\n", (UINT) sci_id);
    
    SCI_CHECK_INIT_COND(sci_id, rc);
    
    if (rc != SCI_ERROR_OK)
    {
        PDEBUG("card[%d] error=%d\n", (UINT) sci_id, rc);
        return rc;
    }
    
    p_sci_parameters->T   = sci_cb[sci_id].sci_parameters.T;
    p_sci_parameters->f   = sci_cb[sci_id].sci_parameters.f;
    p_sci_parameters->ETU = sci_cb[sci_id].sci_parameters.ETU;
    p_sci_parameters->WWT = sci_cb[sci_id].sci_parameters.WWT;
    p_sci_parameters->CWT = sci_cb[sci_id].sci_parameters.CWT;
    p_sci_parameters->BWT = sci_cb[sci_id].sci_parameters.BWT;
    p_sci_parameters->EGT = sci_cb[sci_id].sci_parameters.EGT;

    p_sci_parameters->clock_stop_polarity =
        sci_cb[sci_id].sci_parameters.clock_stop_polarity;
    p_sci_parameters->check =
        sci_cb[sci_id].sci_parameters.check;
    p_sci_parameters->P = sci_cb[sci_id].sci_parameters.P;
    p_sci_parameters->I = sci_cb[sci_id].sci_parameters.I;
    p_sci_parameters->U = sci_cb[sci_id].sci_parameters.U;

    PDEBUG("card[%d] exit\n", (UINT) sci_id);

    return(rc);
}


/*****************************************************************************
** Function:    sci_osi_set_para_T
**
** Purpose:     Set the current Smart Card parameters of T.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
**************************************************************************/
SCI_ERROR sci_osi_set_para_T(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    /* set the protocol T=0 or 1 of sci */
    if((p_sci_parameters->T == 0) || (p_sci_parameters->T == 1))
    {
        if(sci_cb[sci_id].sci_parameters.T != p_sci_parameters->T)
        {
            sci_cb[sci_id].sci_parameters.T = p_sci_parameters->T;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_T\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_f
**
** Purpose:     Set the current Smart Card parameters of f.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/

SCI_ERROR sci_osi_set_para_f(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    /* set the f of sci */
    if((p_sci_parameters->f >= SCI_MIN_F) &&
       (p_sci_parameters->f <= SCI_MAX_F) &&
       (p_sci_parameters->f <= (__STB_SYS_CLK / 2)))
    {
        if(sci_cb[sci_id].sci_parameters.f != p_sci_parameters->f)
        {
            sci_atom_set_para_f(sci_id, p_sci_parameters);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_f\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

 /*****************************************************************************/
static unsigned int ASC_baud_mode_0(unsigned int baud)
{
	unsigned long tmp;
	tmp=(baud*16);
	tmp=(MAIN_FREQ/tmp);
	PDEBUG("ASC_Baud value in MODE 0: %ld\n",tmp);
	return (unsigned int) tmp;
}
/*******/
static unsigned int ASC_baud_mode_1(unsigned int baud)
{
	unsigned long tmp;
	tmp=(baud*16);
	tmp=(tmp/MAIN_FREQ_P1);
	tmp=(tmp*PW_2_16);	/* 2^16 */
	tmp=(tmp/MAIN_FREQ_P2);
	PDEBUG("ASC_Baud value in MODE 1: %ld\n",tmp);
	return (unsigned int) tmp;
}
/*****************************************************************************/
static SCI_ERROR sci_set_para_ETU_table(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
	unsigned int req_baud;
	SCI_ERROR rc = SCI_ERROR_OK;

	if((p_sci_parameters->ETU >= SCI_MIN_ETU) &&
		(p_sci_parameters->ETU <= SCI_MAX_ETU))
	{
		/* This checking is do from the routine set_para_ETU */
//		if(sci_cb[sci_id].sci_parameters.ETU != p_sci_parameters->ETU)
		{
//			sci_cb[sci_id].sci_parameters.ETU = p_sci_parameters->ETU;

			if(sci_cb[sci_id].sci_clock==SCI_CLOCK_3579)
			{
				if(sci_cb[sci_id].sci_parameters.ETU==372)//9600
				{
					switch (sci_id)
					{
						case 0:
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x28B, 0xFFFF);
						break;
						case 1:
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0,0x1000);
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x28B, 0xFFFF);
						break;
					}
				}
				else if(sci_cb[sci_id].sci_parameters.ETU==93)//38400
				{
					switch (sci_id)
					{
						case 0:
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0x1000|1<<12,0);
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x193, 0xFFFF);
						break;
						case 1:
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0x1000|1<<12,0);
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x193, 0xFFFF);
						break;
					}
				}
				else if(sci_cb[sci_id].sci_parameters.ETU==31)//115200
				{
					switch (sci_id)
					{
						case 0:
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0x1000|1<<12,0);
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x4B8, 0xFFFF);
						break;
						case 1:
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0x1000|1<<12,0);
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x4B8, 0xFFFF);
						break;
					}
				}
				else /* no datasheet case -> calculate the new baud rate*/
				{
					req_baud=(CLOCK_3572/sci_cb[sci_id].sci_parameters.ETU);
					PDEBUG(" ### Req_baud: %d\n",req_baud);
					if( (sci_cb[sci_id].sci_parameters.ETU>=360) && (sci_cb[sci_id].sci_parameters.ETU<=384) )
					{
						switch (sci_id)
						{
							case 0:
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
							break;
							case 1:
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0,0x1000);
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
							break;
						}
					}
					else
					{
						switch (sci_id)
						{
							case 0:
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0x1000,0);
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, ASC_baud_mode_1(req_baud), 0xFFFF);
							break;
							case 1:
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0x1000,0);
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, ASC_baud_mode_1(req_baud), 0xFFFF);
							break;
						}
					}
				}
			}
			else if(sci_cb[sci_id].sci_clock==SCI_CLOCK_625)
			{
				if(sci_cb[sci_id].sci_parameters.ETU==625/*372*/)//9600->10000
				{
					switch(sci_id)
					{
						case 0:
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
							_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, 0x271, 0xFFFF);
							debug("slot 0 --> clock 6.25mhz :: baud 10000 :: etu 625 \n");
						break;
						case 1:
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0,0x1000);
							_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, 0x271, 0xFFFF);
							debug("slot 1 --> clock 6.25mhz :: baud 10000 :: etu 625 \n");
						break;
					}
				}
				else /* no datasheet case -> calculate the  new baud rate*/
				{
					req_baud=(CLOCK_625/sci_cb[sci_id].sci_parameters.ETU);
					if( (sci_cb[sci_id].sci_parameters.ETU>=611) && (sci_cb[sci_id].sci_parameters.ETU<=635) )
					{
						switch (sci_id)
						{
							case 0:
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
								_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
							break;
							case 1:
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0,0x1000);
								_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
	}
	return(rc);
}

static void set_req_baud(ULONG s_id,unsigned int r_b)
{
	unsigned int req_baud=r_b;
	
	if(req_baud>THRESHOLD_BAUDRATE)	//mode 1
	{
		switch (s_id)
		{
			case 0:
				_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0x1000,0);
				_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, ASC_baud_mode_1(req_baud), 0xFFFF);
			break;
			case 1:
				_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0x1000,0);
				_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, ASC_baud_mode_1(req_baud), 0xFFFF);
			break;
		}
	}
	else	//mode 0
	{
		switch (s_id)
		{
			case 0:
				_set_reg(0, BASE_ADDRESS_ASC0, ASC0_CTRL,0,0x1000);
				_set_reg(0, BASE_ADDRESS_ASC0, ASC0_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
			break;
			case 1:
				_set_reg(1, BASE_ADDRESS_ASC1, ASC1_CTRL,0,0x1000);
				_set_reg(1, BASE_ADDRESS_ASC1, ASC1_BAUDRATE, ASC_baud_mode_0(req_baud), 0xFFFF);
			break;
		}
	}

}


/*****************************************************************************
** Function:    sci_osi_set_para_ETU
**
** Purpose:     Set the current Smart Card parameters of ETU.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_ETU(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
	unsigned int req_baud;
    SCI_ERROR rc = SCI_ERROR_OK;
	unsigned char action_done=0;

    if((p_sci_parameters->ETU >= SCI_MIN_ETU) &&
       (p_sci_parameters->ETU <= SCI_MAX_ETU))
    {
		/* Beacause in this function we set the parameters ETU and f */
//        if(sci_cb[sci_id].sci_parameters.ETU != p_sci_parameters->ETU)
        {
            sci_cb[sci_id].sci_parameters.ETU = p_sci_parameters->ETU;
			
			if(sci_cb[sci_id].sci_clock==SCI_CLOCK_3579)
			{
				switch(p_sci_parameters->f)
				{
					case 4:
						req_baud=(CLOCK_417/sci_cb[sci_id].sci_parameters.ETU);
						if(sci_id==0)
							_set_reg_writeonly16(0, BASE_ADDRESS_FPGA, FPGA_SCI0_CLOCK, 0x0601);  // == 25Mhz/6*1 = 4.16~ Mhz
						else if(sci_id==1)
							_set_reg_writeonly16(1, BASE_ADDRESS_FPGA, FPGA_SCI1_CLOCK, 0x0601);  // == 25Mhz/6*1 = 4.16~ Mhz

						set_req_baud(sci_id,req_baud);
						smartcard_reset(sci_id, 1);

						action_done=4;
					break;			
					case 5:
						req_baud=(CLOCK_500/sci_cb[sci_id].sci_parameters.ETU);
						if(sci_id==0)
							_set_reg_writeonly16(0, BASE_ADDRESS_FPGA, FPGA_SCI0_CLOCK, 0x0501);  // == 25Mhz/5*1 = 5 Mhz
						else if(sci_id==1)
							_set_reg_writeonly16(1, BASE_ADDRESS_FPGA, FPGA_SCI1_CLOCK, 0x0501);  // == 25Mhz/5*1 = 5 Mhz

						set_req_baud(sci_id,req_baud);
						smartcard_reset(sci_id, 1);

						action_done=5;
					break;	
					case 6:
						req_baud=(CLOCK_625/sci_cb[sci_id].sci_parameters.ETU);

						if(sci_id==0)
							_set_reg_writeonly16(0, BASE_ADDRESS_FPGA, FPGA_SCI0_CLOCK, 0x0401);  // == 25Mhz/4*1 = 6.25 Mhz
						else if(sci_id==1)
							_set_reg_writeonly16(1, BASE_ADDRESS_FPGA, FPGA_SCI1_CLOCK, 0x0401);  // == 25Mhz/4*1 = 6.25 Mhz
						set_req_baud(sci_id,req_baud);
						smartcard_reset(sci_id, 1);

						action_done=6;
					break;
				}
			}

			if(action_done==0)	//use the function with datasheet values
			{
				rc=sci_set_para_ETU_table(sci_id,p_sci_parameters);
			}
		}
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_WWT
**
** Purpose:     Set the current Smart Card parameters of WWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_WWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->WWT >= SCI_MIN_WWT) &&
       (p_sci_parameters->WWT <= SCI_MAX_WWT))
    {
        if(sci_cb[sci_id].sci_parameters.WWT != p_sci_parameters->WWT)
        {
            sci_cb[sci_id].sci_parameters.WWT = p_sci_parameters->WWT;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_WWT\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_CWT
**
** Purpose:     Set the current Smart Card parameters of CWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_CWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->CWT >= SCI_MIN_CWT) &&
       (p_sci_parameters->CWT <= SCI_MAX_CWT))
    {
        if(sci_cb[sci_id].sci_parameters.CWT != p_sci_parameters->CWT)
        {
            sci_cb[sci_id].sci_parameters.CWT = p_sci_parameters->CWT;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_CWT\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_BWT
**
** Purpose:     Set the current Smart Card parameters of BWT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_BWT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->BWT >= SCI_MIN_BWT) &&
       (p_sci_parameters->BWT <= SCI_MAX_BWT))
    {
        if(sci_cb[sci_id].sci_parameters.BWT != p_sci_parameters->BWT)
        {
            sci_cb[sci_id].sci_parameters.BWT = p_sci_parameters->BWT;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_BWT\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_EGT
**
** Purpose:     Set the current Smart Card parameters of EGT.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_EGT(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->EGT >= SCI_MIN_EGT) &&
       (p_sci_parameters->EGT <= SCI_MAX_EGT))
    {
        if(sci_cb[sci_id].sci_parameters.EGT != p_sci_parameters->EGT)
        {
            sci_cb[sci_id].sci_parameters.EGT = p_sci_parameters->EGT;
	        /* EGT: Extra Guard Time...So ADD EGT to GF_DEFAULT (2 stop bit) */
	        _set_reg(sci_id, BASE_ADDRESS_ASC0, ASC0_GUARDTIME, p_sci_parameters->EGT+GT_DEFAULT,0x1FF);
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_CLK_p
**
** Purpose:     Set the current Smart Card parameters of clock stop polarity.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_CLK_p(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->clock_stop_polarity == 0) ||
       (p_sci_parameters->clock_stop_polarity == 1))
    {
        if(sci_cb[sci_id].sci_parameters.clock_stop_polarity !=
            p_sci_parameters->clock_stop_polarity)
        {
            sci_cb[sci_id].sci_parameters.clock_stop_polarity =
                p_sci_parameters->clock_stop_polarity;
        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_CLK_p\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_check
**
** Purpose:     Set the current Smart Card parameters of check.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_check(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;

    if((p_sci_parameters->check == 1) || (p_sci_parameters->check == 2))
    {
        if(sci_cb[sci_id].sci_parameters.check != p_sci_parameters->check)
        {

        }
    }
    else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }

	PDEBUG("Dummy function: sci_osi_set_para_check\n");
	rc=SCI_ERROR_OK;
    return(rc);
}

/*****************************************************************************
** Function:    sci_osi_set_para_class
**
** Purpose:     Set the current Smart Card parameters of class.
**
** Parameters:  sci_id: zero-based number to identify smart card controller
**              p_sci_parameters: input pointer to Smart Card parameters
**
** Returns:     SCI_ERROR_OK: if successful
**              SCI_ERROR_PARAMETER_OUT_OF_RANGE: if sci_id is invalid or
**                  p_sci_parameters is zero.
*****************************************************************************/
SCI_ERROR sci_osi_set_para_class(ULONG sci_id, SCI_PARAMETERS *p_sci_parameters)
{
    SCI_ERROR rc = SCI_ERROR_OK;
	sci_cb[sci_id].sci_parameters.U = p_sci_parameters->U;
	
	if(p_sci_parameters->U == SCI_CLASS_A)
    {
		if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_B)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'A' class type but the ATR is 'B' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_AB)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'A' class type but the ATR is 'AB' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else
			PDEBUG("Card %ld is set 5 V\n",sci_id);
	}
	else if(p_sci_parameters->U == SCI_CLASS_B)
    {
		if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_A)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'B' class type but the ATR is 'A' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_AB)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'B' class type but the ATR is 'AB' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else
			PDEBUG("Card %ld is set 3 V\n",sci_id);
	}
	else if(p_sci_parameters->U == SCI_CLASS_AB)
    {
		if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_A)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'AB' class type but the ATR is 'A' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else if(sci_cb[sci_id].sci_atr_class == SCI_CLASS_B)
		{
			smartcard_voltage_config(sci_id, SCI_VCC_3);
			PDEBUG("WARNING: the parameter U is 'AB' class type but the ATR is 'B' class type\n");
			PDEBUG("WARNING: Set 3 V\n");
		}
		else
			PDEBUG("Card %ld is set 3 V\n",sci_id);
	}
	else if(p_sci_parameters->U == SCI_UNKNOWN_CLASS)
    {
		PDEBUG("Unknown class, doesn't set voltage\n");
		rc=SCI_ERROR_OK;
	}
	else
    {
        rc=SCI_ERROR_PARAMETER_OUT_OF_RANGE;
    }
	return(rc);

}


