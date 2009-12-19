////////////////////////////////////////////////////////////////////////////
/// COPYRIGHT (C) STMicroelectronics 2007
///
/// Source file name : mixer_mme.h
/// Author :           Daniel
///
/// Concrete definition of an MME mixer driver.
///
///
/// Date        Modification                                    Name
/// ----        ------------                                    --------
/// 29-Jun-07   Created                                         Daniel
///
////////////////////////////////////////////////////////////////////////////

#ifndef H_MIXER_CLASS
#define H_MIXER_CLASS

#include "player_types.h"

// /////////////////////////////////////////////////////////////////////////
//
// Macro definitions.
//

#ifndef ENABLE_MIXER_DEBUG
#define ENABLE_MIXER_DEBUG 0
#endif

#define MIXER_TAG                          "Mixer_c::"
#define MIXER_FUNCTION                     __FUNCTION__

/* Output debug information (which may be on the critical path) but is usually turned off */
#define MIXER_DEBUG(fmt, args...) ((void)(ENABLE_MIXER_DEBUG && \
					  (report(severity_note, "%s%s: " fmt, MIXER_TAG, MIXER_FUNCTION, ##args), 0)))

/* Output trace information off the critical path */
#define MIXER_TRACE(fmt, args...) (report(severity_note, "%s%s: " fmt, MIXER_TAG, MIXER_FUNCTION, ##args))
/* Output errors, should never be output in 'normal' operation */
#define MIXER_ERROR(fmt, args...) (report(severity_error, "%s%s: " fmt, MIXER_TAG, MIXER_FUNCTION, ##args))

#define MIXER_ASSERT(x) do if(!(x)) report(severity_error, "%s: Assertion '%s' failed at %s:%d\n", \
					       MIXER_FUNCTION, #x, __FILE__, __LINE__); while(0)


class Mixer_c:public BaseComponentClass_c
{
  public:
    enum InputState_t
    {
	/// There is no manifestor connected to this input.
	DISCONNECTED,

	/// Neither the input nor the output side are running.
	STOPPED,

	/// Manifestor has been enabled but we have yet to receive parameters from it.
	UNCONFIGURED,

	/// Manifestor has been configured and we are waiting for the mixer loop to start using it.
	STARTING,

	/// The output side is primed and ready but might not be sending (muted)
	/// samples to the speakers yet.
	STARTED,

	/// Manifestor has been disabled but we have not yet stopped mixing from it.
	STOPPING,
    };

    static inline const char *LookupInputState( InputState_t state )
    {
	switch ( state )
	{
#define E(x) case x: return #x
	    E( DISCONNECTED );
	    E( STOPPED );
	    E( UNCONFIGURED );
	    E( STARTING );
	    E( STARTED );
	    E( STOPPING );
#undef E
	    default:return "INVALID";
	}
    }
};

#endif // H_MIXER_CLASS
