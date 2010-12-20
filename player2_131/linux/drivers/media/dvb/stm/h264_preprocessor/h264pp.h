/******************************************************************************
 * h264pp.h
 *
 * Include file for the h264 preprocessor registers
 *
 ******************************************************************************
 * Copyright (c) 2005 STMicroelectronics Limited, All rights reserved.
 *****************************************************************************/
#ifndef _H264PP_H_
#define _H264PP_H_

//

#define H264_PP_MAX_SLICES	256					// Need 137 in principle
#define H264_PP_SESB_SIZE       (8 * H264_PP_MAX_SLICES)		// 2 words per slice
#define H264_PP_OUTPUT_SIZE     (0x00180000 - H264_PP_SESB_SIZE)	// 1.5 Mb - SESB data
#define H264_PP_INPUT_SIZE      0x00100000				// 1.0 Mb for collating the input to the PP
#define H264_PP_BUFFER_SIZE     (H264_PP_SESB_SIZE + H264_PP_OUTPUT_SIZE + H264_PP_INPUT_SIZE)

#define H264_PP_RESET_TIME_LIMIT	100

//

#define H264_PP_MAX_SUPPORTED_BUFFERS   8
#define H264_PP_MAX_SUPPORTED_PP        2
#define H264_PP_PER_INSTANCE            h264_pp_per_instance
#define H264_PP_PER_INSTANCE_MASK       ((1<<H264_PP_PER_INSTANCE) - 1)
#define H264_PP_REGISTER_SIZE           256

//#define H264_PP_INTERRUPT(N)                    (149 + (N))

#define H264_PP_NUMBER_OF_PRE_PROCESSORS        h264_pp_number_of_pre_processors
#define H264_PP_REGISTER_BASE(N)                h264_pp_register_base[N]
#define H264_PP_INTERRUPT(N)                    (h264_pp_interrupt[N])

//

#if defined (__KERNEL__)
#ifndef H264_PP_REGISTER_BASE
#error No chip defined, unable to determine register addresses for H264 PP
#endif
#endif

#ifndef H264_PP_MAPPED_REGISTER_BASE
#define H264_PP_MAPPED_REGISTER_BASE            MappedRegisterBaseNotDefined
#endif

//

#define PP_BASE(N)                    (H264_PP_MAPPED_REGISTER_BASE[N])

#define PP_READ_START(N)              (PP_BASE(N) + 0x0000)
#define PP_READ_STOP(N)               (PP_BASE(N) + 0x0004)
#define PP_BBG(N)                     (PP_BASE(N) + 0x0008)
#define PP_BBS(N)                     (PP_BASE(N) + 0x000c)
#define PP_ISBG(N)                    (PP_BASE(N) + 0x0010)
#define PP_IPBG(N)                    (PP_BASE(N) + 0x0014)
#define PP_IBS(N)                     (PP_BASE(N) + 0x0018)
#define PP_WDL(N)                     (PP_BASE(N) + 0x001c)
#define PP_CFG(N)                     (PP_BASE(N) + 0x0020)
#define PP_PICWIDTH(N)                (PP_BASE(N) + 0x0024)
#define PP_CODELENGTH(N)              (PP_BASE(N) + 0x0028)
#define PP_START(N)                   (PP_BASE(N) + 0x002c)
#define PP_MAX_OPC_SIZE(N)            (PP_BASE(N) + 0x0030)
#define PP_MAX_CHUNK_SIZE(N)          (PP_BASE(N) + 0x0034)
#define PP_MAX_MESSAGE_SIZE(N)        (PP_BASE(N) + 0x0038)
#define PP_ITS(N)                     (PP_BASE(N) + 0x003c)
#define PP_ITM(N)                     (PP_BASE(N) + 0x0040)
#define PP_SRS(N)                     (PP_BASE(N) + 0x0044)
#define PP_DFV_OUTCTRL(N)             (PP_BASE(N) + 0x0048)


/* PP bit fields/shift values */

#define PP_CFG__CONTROL_MODE__START_STOP        0x00000000
#define PP_CFG__CONTROL_MODE__START_PAUSE       0x10000000
#define PP_CFG__CONTROL_MODE__RESTART_STOP      0x20000000
#define PP_CFG__CONTROL_MODE__RESTART_PAUSE     0x30000000

#define PP_CFG__MONOCHROME                      31              // Not in the documentation
#define PP_CFG__DIRECT8X8FLAG_SHIFT             30              // Not in the documentation
#define PP_CFG__TRANSFORM8X8MODE_SHIFT          27              // Not in the documentation
#define PP_CFG__QPINIT_SHIFT                    21
#define PP_CFG__IDXL1_SHIFT                     16
#define PP_CFG__IDXL0_SHIFT                     11
#define PP_CFG__DEBLOCKING_SHIFT                10
#define PP_CFG__BIPREDFLAG_SHIFT                9
#define PP_CFG__PREDFLAG_SHIFT                  8
#define PP_CFG__DPOFLAG_SHIFT                   6
#define PP_CFG__POPFLAG_SHIFT                   5
#define PP_CFG__POCTYPE_SHIFT                   3
#define PP_CFG__FRAMEFLAG_SHIFT                 2
#define PP_CFG__ENTROPYFLAG_SHIFT               1
#define PP_CFG__MBADAPTIVEFLAG_SHIFT            0

#define PP_CODELENGTH__MPOC_SHIFT               5
#define PP_CODELENGTH__MFN_SHIFT                0

#define PP_PICWIDTH__MBINPIC_SHIFT              16
#define PP_PICWIDTH__PICWIDTH_SHIFT             0

#define PP_ITM__WRITE_ERROR                     0x00000100	// error has been detected when writing the pre-processing result in external memory.
#define PP_ITM__READ_ERROR                      0x00000080	// error has been detected when reading the compressed data from external memory.
#define PP_ITM__BIT_BUFFER_OVERFLOW             0x00000040	//
#define PP_ITM__BIT_BUFFER_UNDERFLOW            0x00000020	// Read stop address reached (PP_BBS), picture decoding not finished
#define PP_ITM__INT_BUFFER_OVERFLOW             0x00000010	// Write address for intermediate buffer reached PP_IBS
#define PP_ITM__ERROR_BIT_INSERTED              0x00000008	// Error bit has been inserted in Slice Error Status Buffer
#define PP_ITM__ERROR_SC_DETECTED               0x00000004	// Error Start Code has been detected
#define PP_ITM__SRS_COMP                        0x00000002	// Soft reset is completed
#define PP_ITM__DMA_CMP                         0x00000001	// Write DMA is completed

//

#endif
