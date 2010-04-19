#ifdef UFS912
#ifndef _UFS912_CIC_H_
#define _UFS912_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define cNumberSlots   2

struct ufs912_cic_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
};

struct ufs912_cic_state {
        struct dvb_frontend_ops                 ops;
	struct ufs912_cic_core		        *core;

        struct i2c_adapter      		*i2c;
        int					i2c_addr;

	struct stpio_pin			*slot_enable[cNumberSlots];
	struct stpio_pin			*slot_reset[cNumberSlots];
	struct stpio_pin			*slot_status[cNumberSlots];

        int                                     module_ready[cNumberSlots];
        int                                     module_present[cNumberSlots];
        unsigned long                           detection_timeout[cNumberSlots];
};

int init_ufs912_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
#endif
