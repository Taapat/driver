/************************************************************************
COPYRIGHT (C) SGS-THOMSON Microelectronics 1996

Source file name : report.h
Author :           Nick

    Contains the header file for the report functions,
    The actual reporting function, and a function to restrict
    the range of severities reported (NOTE Fatal and greater
    messages are always reported). It is upto the caller to
    insert file/line number information into the report. Process
    information will be prepended to any report of severity error
    or greater.

Date        Modification                                    Name
----        ------------                                    ----
18-Dec-96   Created                                         nick
************************************************************************/

#ifndef __REPORT_H
#define __REPORT_H

#ifdef __cplusplus
extern "C" {
#endif

//

#define REPORT_STRING_SIZE      256

typedef enum
    {
	severity_info           = 0,
	severity_note           = 50,
	severity_error          = 100,
	severity_fatal          = 200,
	severity_interrupt      = 1000
    } report_severity_t;

void report_init( void );

void report_restricted_severity_levels( int lower_restriction,
					int upper_restriction );

void report( report_severity_t   report_severity,
	     const char         *format, ... );

#ifdef REPORT
void report_dump_hex( report_severity_t level,
		      unsigned char    *data,
		      int               length,
		      int               width,
		      void*             start );
#else
#define report_dump_hex(a,b,c,d,e) do { } while(0)
#endif

#if 0
#define DEBUG_EVENT_BASE 0x0b000000

static unsigned int *debug_base= (unsigned int *)DEBUG_EVENT_BASE;
static unsigned int *debug_data= (unsigned int *)(DEBUG_EVENT_BASE + 0x40);

#define initialise_debug_event()        {volatile int i; for( i=0; i<0x2000; i++ ) debug_base[i] = 0; i = debug_base[16]; }

#define print_debug_events()            {int i; for( i=0; i<(512+16); i+=8 )    \
						report( severity_info, "        %08x %08x %08x %08x %08x %08x %08x %08x\n",     \
						debug_base[i+0],debug_base[i+1],debug_base[i+2],debug_base[i+3],        \
						debug_base[i+4],debug_base[i+5],debug_base[i+6],debug_base[i+7] ); }

//#define debug_event(code)            if( debug_base[15] != 0xfeedface ) {volatile unsigned int dummy; dummy=debug_base[0]; debug_data[dummy++] = (unsigned int)code; dummy &= 0x1ff; debug_data[dummy] = 0xffffffff; debug_base[0] = dummy; dummy = debug_base[32];} else { while(true) task_delay(100); }
#define debug_event(code)              {volatile unsigned int dummy; dummy=debug_base[0]; debug_data[dummy++] = (unsigned int)code; dummy &= 0x1ff; debug_data[dummy] = 0xffffffff; debug_base[0] = dummy; }


#else
#define initialise_debug_event()
#define print_debug_events()
#define debug_event(code)
#endif

//

#ifdef __cplusplus
}
#endif
#endif
