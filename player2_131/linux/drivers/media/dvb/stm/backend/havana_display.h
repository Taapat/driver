/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : havana_display.h derived from havana_player_factory.h
Author :           Julian

Definition of the implementation of havana player module for havana.


Date        Modification                                    Name
----        ------------                                    --------
05-Sep-07   Created                                         Julian

************************************************************************/

#ifndef H_HAVANA_DISPLAY
#define H_HAVANA_DISPLAY

#define FACTORY_ANY_ID                  "*"

#if defined(__TDT__)

#if defined(FORTIS_HDBOX) || defined(TF7700) || defined(UFS922) || defined(HL101) || defined(VIP1_V2) || defined(VIP2_V1) || defined(UFS912) || defined(SPARK) || defined(SPARK7162) || defined(OCTAGON1008)  || defined(CUBEREVO) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_MINI) || defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_9500HD) || defined(CUBEREVO_MINI_FTA) || defined(ATEVIO7500)
#define AUDIO_BUFFER_MEMORY                     0x00180000
#else
#define AUDIO_BUFFER_MEMORY                     0x00100000       // 1 mb
#endif

/* When having 64 MB in the LMI_VID the upper limit for the primary
   video buffer seems to be 29.5 MB (enough for 9.9 full HD 4:2:0).
   However, then all memory in LMI_VID is used by the primary video
   buffer & co. The lower limit for HD display is 15 MB. Below that
   value the playback is frequently disturbed. The secondary video
   buffer can be used for PiP. The firmware (transformers) seems to
   support PiP with MPEG2 (SD) sources only. Therefore, the secondary
   buffer does not need much memory.
   The total amount of memory allocated for the primary and secondary
   buffers should be 1-2 MB less than the maximum described above
   because of memory fragmentation. Otherwise the player cannot get
   memory for auxiliary buffers. */
#if defined(UFS922) || defined(TF7700) || defined(FORTIS_HDBOX) || defined(HL101) || defined(VIP1_V2) || defined(VIP2_V1) || defined(OCTAGON1008) || defined(CUBEREVO) || defined(CUBEREVO_MINI2) || defined(CUBEREVO_MINI) || defined(CUBEREVO_250HD) || defined(CUBEREVO_2000HD) || defined(CUBEREVO_9500HD) || defined(CUBEREVO_MINI_FTA)

#define PRIMARY_VIDEO_BUFFER_MEMORY             0x01500000       // 21 mb or enough for 7 full HD 4:2:0
#define SECONDARY_VIDEO_BUFFER_MEMORY           0x00300000       // 3 mb
#define AVR_VIDEO_BUFFER_MEMORY                 0x00000000       // 0 mb
#define MAX_VIDEO_DECODE_BUFFERS                32

#elif defined(UFS912) || defined(SPARK) || defined(SPARK7162) || defined(ATEVIO7500)

#define PRIMARY_VIDEO_BUFFER_MEMORY             0x02400000       // 36 mb or enough for 12 full hd 4:2:0
/* The pip setting is not checked, I dont have compiled e2 to test it.
 * I'm not sure what is necessary for hd pip, but I think it must be
 * possible on ufs912. This memory is enough for 3 full hd 4:2:0
 * picture.
 */
#define SECONDARY_VIDEO_BUFFER_MEMORY           0x00900000       // 9 mb
#define AVR_VIDEO_BUFFER_MEMORY                 0x00000000       // 0 mb
#define MAX_VIDEO_DECODE_BUFFERS                32

#else /* UFS910 and others */

#define PRIMARY_VIDEO_BUFFER_MEMORY             0x01B00000       // 27 mb or enough for 9 full HD 4:2:0
#define SECONDARY_VIDEO_BUFFER_MEMORY           0x00000000       // 0 mb
#define AVR_VIDEO_BUFFER_MEMORY                 0x00000000       // 0 mb
#define MAX_VIDEO_DECODE_BUFFERS                32

#endif

#else /* not __TDT__ */

#define AUDIO_BUFFER_MEMORY                     0x00080000       // 1/2   mb

#if defined (CONFIG_CPU_SUBTYPE_STX7200) && defined (CONFIG_32BIT)

/* This assumes a 128MB LMI1 as currently used on the MB519 and cb101 */
#define PRIMARY_VIDEO_BUFFER_MEMORY             0x01B00000       // 27 mb or enough for 9 full HD 4:2:0
#define SECONDARY_VIDEO_BUFFER_MEMORY           0x00800000       // 22 mb or enough for  7 full HD 4:2:0
#define AVR_VIDEO_BUFFER_MEMORY                 0x06c00000       // 108 mb
#define MAX_VIDEO_DECODE_BUFFERS                32

#else

#define PRIMARY_VIDEO_BUFFER_MEMORY             0x02700000       // 39 mb or enough for 13 full HD 4:2:0
#define SECONDARY_VIDEO_BUFFER_MEMORY           0x00000000       // 0 mb or enough for 6 full HD 4:2:0
#define AVR_VIDEO_BUFFER_MEMORY                 0x02F00000
#define MAX_VIDEO_DECODE_BUFFERS                32

#endif /* 7200 & 32Bit mode */

#endif

/* Position and size of PIP Window - based on the display size */
#define PIP_FACTOR_N                            5
#define PIP_FACTOR_D                            16
#define PIP_X_N                                (PIP_FACTOR_D-PIP_FACTOR_N)
#define PIP_X_OFFSET                            48
#define PIP_Y_OFFSET                            32

/*      Debug printing macros   */
#ifndef ENABLE_DISPLAY_DEBUG
#define ENABLE_DISPLAY_DEBUG            0
#endif

#define DISPLAY_DEBUG(fmt, args...)      ((void) (ENABLE_DISPLAY_DEBUG && \
                                            (report(severity_note, "HavanaDisplay_c::%s: " fmt, __FUNCTION__, ##args), 0)))

/* Output trace information off the critical path */
#define DISPLAY_TRACE(fmt, args...)     (report(severity_note, "HavanaDispla_c::%s: " fmt, __FUNCTION__, ##args))
/* Output errors, should never be output in 'normal' operation */
#define DISPLAY_ERROR(fmt, args...)     (report(severity_error, "HavanaDisplay_c::%s: " fmt, __FUNCTION__, ##args))


/// Display wrapper class responsible for managing manifestors.
class HavanaDisplay_c
{
private:
    DeviceHandle_t              DisplayDevice;

    class Manifestor_c*         Manifestor;
    PlayerStreamType_t          PlayerStreamType;

public:

                                HavanaDisplay_c                (void);
                               ~HavanaDisplay_c                (void);

    HavanaStatus_t              GetManifestor                  (class HavanaPlayer_c*   HavanaPlayer,
                                                                char*                   Media,
                                                                char*                   Encoding,
                                                                unsigned int            SurfaceId,
                                                                class Manifestor_c**    Manifestor);

    class Manifestor_c*         ReferenceManifestor             (void) {return Manifestor;};
};

/*{{{  doxynote*/
/*! \class      HavanaDisplay_c
    \brief      Controls access to and initialisation of the manifestor

*/

/*! \fn class Manifestor_c* HavanaDisplay_c::GetManifestor( void );
\brief Create and initialise a manifestor.

    Create a manifestor and initialise it based on the input parameters.

\param HavanaPlayer     Parent class
\param Media            Video or Audio
\param Encoding         Content type - determines some layout characteristic
\param SurfaceId        Where we are going to display the stream
\param Manifestor       Created manifestor

\return Havana status code, HavanaNoError indicates success.
*/

/*! \fn class Manifestor_c* HavanaDisplay_c::ReferenceManifestor( void );
\brief Use the manifestor that this class holds.

    Use the current manifestor, without perturbing it (ie setting parameters encoding etc...).

\return a pointer to a manifestor class instance.
*/



/*}}}  */

#endif

