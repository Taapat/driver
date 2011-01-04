#ifndef _UFS9XX_CIC_H_
#define _UFS9XX_CIC_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

#define cNumberSlots   2

struct ufs9xx_cic_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
};

struct ufs9xx_cic_state {
        struct dvb_frontend_ops                 ops;
	struct ufs9xx_cic_core		        *core;

#ifdef UFS922
        struct i2c_adapter      		*i2c;
        int					i2c_addr;
#endif

	struct stpio_pin			*slot_enable[cNumberSlots];
	struct stpio_pin			*slot_reset[cNumberSlots];
	struct stpio_pin			*slot_status[cNumberSlots];

        int                                     module_ready[cNumberSlots];
        int                                     module_present[cNumberSlots];
        unsigned long                           detection_timeout[cNumberSlots];

#ifdef UFS922
	/* which tuner to which ca? */
	int					module_a_source;
	int					module_b_source;
#endif
};

int init_ufs9xx_cic(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);
void getCiSource(int slot, int* source);

#endif
