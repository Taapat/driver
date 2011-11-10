#ifndef lnb_123
#define lnb_123

#define cLNB_LNBH221 1
#define cLNB_PIO     2

#include "equipment.h"

extern void* lnbh221_attach(u32* lnb, struct equipment_s* equipment);
extern void* lnb_pio_attach(u32* lnb, struct equipment_s* equipment);

#endif
