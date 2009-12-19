/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : dvb_dmux.h - demux device definitions
Author :           Julian

Date        Modification                                    Name
----        ------------                                    --------
01-Nov-06   Created                                         Julian

************************************************************************/

#ifndef H_DVB_DEMUX
#define H_DVB_DEMUX

#include "dvbdev.h"

#if 0
int DmxWrite   (struct dmx_demux*       Demux,
                const char*             Buffer,
                size_t                  Count);
#endif
int StartFeed  (struct dvb_demux_feed*  Feed);
int StopFeed   (struct dvb_demux_feed*  Feed);
#ifdef __TDT__ 
int WriteToDecoder (struct dvb_demux_feed *Feed, const u8 *buf, size_t count);
#endif

#endif
