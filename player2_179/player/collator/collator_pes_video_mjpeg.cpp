/************************************************************************
COPYRIGHT (C) STMicroelectronics 2009

Source file name : collator_pes_video_mjpeg.cpp
Author :           Julian

Implementation of the pes collator class for player 2.


Date        Modification                                    Name
----        ------------                                    --------
30-Jul-09   Created                                         Julian

************************************************************************/

////////////////////////////////////////////////////////////////////////////
/// \class Collator_PesVideoMjpeg_c
///
/// Implements initialisation of collator video class for MJPEG
///

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "collator_pes_video_mjpeg.h"
#include "mjpeg.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined constants
//

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

//{{{  Constructor
////////////////////////////////////////////////////////////////////////////
///
/// Initialize the class by resetting it.
///
/// During a constructor calls to virtual methods resolve to the current class (because
/// the vtable is still for the class being constructed). This means we need to call
/// ::Reset again because the calls made by the sub-constructors will not have called
/// our reset method.
///
Collator_PesVideoMjpeg_c::Collator_PesVideoMjpeg_c( void )
{
    COLLATOR_TRACE("%s\n", __FUNCTION__);
    if( InitializationStatus != CollatorNoError )
        return;

    Collator_PesVideoMjpeg_c::Reset();
}
//}}}
//{{{  Reset
////////////////////////////////////////////////////////////////////////////
///
/// Resets and configures according to the requirements of this stream content
///
/// \return void
///
CollatorStatus_t Collator_PesVideoMjpeg_c::Reset( void )
{
    CollatorStatus_t Status;

    COLLATOR_TRACE("%s\n", __FUNCTION__);

    Status = Collator_PesVideo_c::Reset();
    if( Status != CollatorNoError )
        return Status;

    Configuration.GenerateStartCodeList         = true;
    Configuration.MaxStartCodes                 = 32;

    Configuration.StreamIdentifierMask          = PES_START_CODE_MASK;
    Configuration.StreamIdentifierCode          = PES_START_CODE_VIDEO;
    Configuration.BlockTerminateMask            = 0xff;
    Configuration.BlockTerminateCode            = MJPEG_SOI;
    Configuration.IgnoreCodesRangeStart         = 0x00;
    Configuration.IgnoreCodesRangeEnd           = MJPEG_SOF_0 - 1;
    Configuration.InsertFrameTerminateCode      = false;                            // Force the mme decode to terminate after a picture
    Configuration.TerminalCode                  = 0;
    Configuration.ExtendedHeaderLength          = 0;

    Configuration.DeferredTerminateFlag         = false;

    Configuration.StreamTerminateFlushesFrame   = false;         // Use an end of sequence to force a frame flush
    Configuration.StreamTerminationCode         = 0;

    return CollatorNoError;
}
//}}}
//{{{  FindNextStartCode
// /////////////////////////////////////////////////////////////////////////
//
//      Protected - Find the next start code (apart from any one at offset 0)
//      start codes are of the form "ff xx"

CollatorStatus_t   Collator_PesVideoMjpeg_c::FindNextStartCode(unsigned int             *CodeOffset )
{
    unsigned char       IgnoreLower;
    unsigned char       IgnoreUpper;

    //
    // If less than 2 bytes we do not bother
    //

    if( RemainingLength < 2 )
        return CollatorError;

    IgnoreLower                 = Configuration.IgnoreCodesRangeStart;
    IgnoreUpper                 = Configuration.IgnoreCodesRangeEnd;

    for (unsigned int i=0; i<(RemainingLength-2); i++)
        if (RemainingData[i] == 0xff)
        {
            if (inrange(RemainingData[i+1], IgnoreLower, IgnoreUpper) || (RemainingData[i+1]==0xff) )
                continue;

            *CodeOffset         = i;
            return CollatorNoError;
        }

    return CollatorError;
}
//}}}
//{{{  Input
////////////////////////////////////////////////////////////////////////////
///
/// Extract Frame length from Pes Private Data area if present.
///
/// \return Collator status code, CollatorNoError indicates success.
///
CollatorStatus_t   Collator_PesVideoMjpeg_c::Input     (PlayerInputDescriptor_t*        Input,
                                                        unsigned int                    DataLength,
                                                        void*                           Data)
{
    CollatorStatus_t    Status                  = CollatorNoError;
    unsigned char*      DataBlock               = (unsigned char*)Data;
    unsigned char*      PesHeader;
    unsigned int        PesLength;
    unsigned int        PayloadLength;
    unsigned int        Offset;
    unsigned int        CodeOffset;
    unsigned int        Code;

    AssertComponentState( "Collator_PacketPes_c::Input", ComponentRunning );

    Offset                      = 0;
    RemainingData               = (unsigned char *)Data;
    RemainingLength             = DataLength;
    TerminationFlagIsSet        = false;

    Offset                      = 0;
    while (Offset < DataLength)
    {
        // Read the length of the payload
        PesHeader               = DataBlock + Offset;
        PesLength               = (PesHeader[4] << 8) + PesHeader[5];
        if (PesLength != 0)
            PayloadLength       = PesLength - PesHeader[8] - 3;
        else
            PayloadLength       = 0;
        COLLATOR_DEBUG("DataLength %d, PesLength %d; PayloadLength %d, Offset %d\n", DataLength, PesLength, PayloadLength, Offset);
        Offset                 += PesLength + 6;        // PES packet is PesLength + 6 bytes long

        Bits.SetPointer (PesHeader + 9);                // Set bits pointer ready to process optional fields
        if ((PesHeader[7] & 0x80) == 0x80)              // PTS present?
        //{{{  read PTS
        {
            Bits.FlushUnseen(4);
            PlaybackTime        = (unsigned long long)(Bits.Get(3)) << 30;
            Bits.FlushUnseen(1);
            PlaybackTime       |= Bits.Get(15) << 15;
            Bits.FlushUnseen(1);
            PlaybackTime       |= Bits.Get(15);
            Bits.FlushUnseen(1);
            PlaybackTimeValid   = true;
            COLLATOR_DEBUG("PTS %llu.\n", PlaybackTime );
        }
        //}}}
        if ((PesHeader[7] & 0xC0) == 0xC0)              // DTS present?
        //{{{  read DTS
        {
            Bits.FlushUnseen(4);
            DecodeTime          = (unsigned long long)(Bits.Get(3)) << 30;
            Bits.FlushUnseen(1);
            DecodeTime         |= Bits.Get(15) << 15;
            Bits.FlushUnseen(1);
            DecodeTime         |= Bits.Get(15);
            Bits.FlushUnseen(1);

            DecodeTimeValid     = true;
        }
        //}}}
        else if ((PesHeader[7] & 0xC0) == 0x40)
        {
            COLLATOR_ERROR("Malformed pes header contains DTS without PTS.\n" );
            DiscardAccumulatedData();                   // throw away previous frame as incomplete
            return CollatorError;
        }

        RemainingData           = PesHeader + (PesLength + 6 - PayloadLength);
        RemainingLength         = PayloadLength;
        while (RemainingLength > 0)
        {
            Status              = FindNextStartCode (&CodeOffset);
            if (Status != CollatorNoError)              // Error indicates no start code found
            {
                Status          = AccumulateData (RemainingLength, RemainingData);
                if (Status != CollatorNoError)
                    DiscardAccumulatedData();

                RemainingLength = 0;
                break;
            }

            // Got one accumulate upto and including it
            Status              = AccumulateData (CodeOffset+2, RemainingData);
            if (Status != CollatorNoError )
            {
                DiscardAccumulatedData();
                break;
            }

            Code                        = RemainingData[CodeOffset+1];
            RemainingLength            -= CodeOffset+2;
            RemainingData              += CodeOffset+2;

            // Is it a block terminate code
            if ((Code == Configuration.BlockTerminateCode) && (AccumulatedDataSize > 2))
            {
                AccumulatedDataSize    -= 2;

                Status          = FrameFlush ((Configuration.StreamTerminateFlushesFrame && (Code == Configuration.StreamTerminationCode)));
                if (Status != CollatorNoError)
                    break;

                BufferBase[0]           = 0xff;
                BufferBase[1]           = Code;
                AccumulatedDataSize     = 2;
            }

            // Accumulate the start code
            Status                      = AccumulateStartCode (PackStartCode(AccumulatedDataSize-2,Code));
            if( Status != CollatorNoError )
            {
                DiscardAccumulatedData();
                return Status;
            }
        }
    }

    return CollatorNoError;
}
//}}}

