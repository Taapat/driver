#ifndef __CMD_OSDEP_H_
#define __CMD_OSDEP_H_


#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

extern sint _init_cmd_priv (struct	cmd_priv *pcmdpriv);
extern sint _init_evt_priv(struct evt_priv *pevtpriv);
extern void _free_evt_priv (struct	evt_priv *pevtpriv);
extern void _free_cmd_priv (struct	cmd_priv *pcmdpriv);
extern sint _enqueue_cmd(_queue *queue, struct cmd_obj *obj);
extern struct	cmd_obj	*_dequeue_cmd(_queue *queue);

#endif

