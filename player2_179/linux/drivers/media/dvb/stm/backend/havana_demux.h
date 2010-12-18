/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : havana_demux.h derived from havana_player.h
Author :           Julian

Definition of the implementation of demux module for havana.


Date        Modification                                    Name
----        ------------                                    --------
17-Apr-07   Created                                         Julian

************************************************************************/

#ifndef H_HAVANA_DEMUX
#define H_HAVANA_DEMUX

#include "osdev_user.h"
#include "player.h"
#include "player_generic.h"
#include "havana_playback.h"

/*      Debug printing macros   */
#ifndef ENABLE_DEMUX_DEBUG
#define ENABLE_DEMUX_DEBUG             1
#endif

#define DEMUX_DEBUG(fmt, args...)      ((void) (ENABLE_DEMUX_DEBUG && \
                                            (report(severity_note, "HavanaDemux_c::%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define DEMUX_TRACE(fmt, args...)      (report(severity_note, "HavanaDemux_c::%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define DEMUX_ERROR(fmt, args...)      (report(severity_error, "HavanaDemux_c::%s: " fmt, __FUNCTION__, ##args))


/// Player wrapper component to manage demultiplexing.
class HavanaDemux_c
{
private:
    OS_Mutex_t                  InputLock;

    class Player_c*             Player;
    PlayerPlayback_t            PlayerPlayback;

    DemultiplexorContext_t      DemuxContext;

public:

                                HavanaDemux_c                  (void);
                               ~HavanaDemux_c                  (void);

    HavanaStatus_t              Init                           (class Player_c*                 Player,
                                                                PlayerPlayback_t                PlayerPlayback,
                                                                DemultiplexorContext_t          DemultiplexorContext);
    HavanaStatus_t              InjectData                     (const unsigned char*            Data,
                                                                unsigned int                    DataLength);
};

#endif

