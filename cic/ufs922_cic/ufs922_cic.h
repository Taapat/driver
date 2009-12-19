#ifdef UFS922
#ifndef _UFS922_CIC_H_
#define _UFS922_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

struct ufs922_cic_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
};

struct ufs922_cic_state {
        struct dvb_frontend_ops                 ops;
	struct ufs922_cic_core		        *core;

        struct i2c_adapter      		*i2c;
        int					i2c_addr;
	struct stpio_pin			*enable_pin;
	struct stpio_pin			*slot_a;
	struct stpio_pin			*slot_b;
	struct stpio_pin			*slot_a_enable;
	struct stpio_pin			*slot_b_enable;
	struct stpio_pin			*slot_a_busy;
	struct stpio_pin			*slot_b_busy;
	struct stpio_pin			*slot_a_status;
	struct stpio_pin			*slot_b_status;
	
        int                                     module_a_present;
        int                                     module_b_present;

	/* which tuner to which ca? */
	int					module_a_source;
	int					module_b_source;
};

int init_ufs922_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
#endif
