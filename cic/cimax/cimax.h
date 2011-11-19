#ifndef _CIMAX_H_
#define _CIMAX_H_

#include "dvb_ca_en50221.h"
#include "dvb_frontend.h"

struct cimax_core {
        struct dvb_adapter		*dvb_adap;
        struct dvb_ca_en50221           ca; /* cimax */
        struct dvb_ca_en50221           ca1; /* for camd access */
};

struct cimax_state {
        struct dvb_frontend_ops                 ops;
	struct cimax_core			*core;

        struct i2c_adapter      		*i2c;
        int					i2c_addr;
        int                                     cimax_module_present[2];
        unsigned long                           detection_timeout[2];
};

int init_cimax(struct dvb_adapter *dvb_adap);

int setCiSource(int slot, int source);

#endif
