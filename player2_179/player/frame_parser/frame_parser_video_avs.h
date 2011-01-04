/************************************************************************
COPYRIGHT (C) ST Microelectronics 2009

Source file name : frame_parser_video_avs.h
Author :           Julian

Definition of the frame parser video AVS class implementation for player 2.


Date        Modification                                    Name
----        ------------                                    --------

************************************************************************/

#ifndef H_FRAME_PARSER_VIDEO_AVS
#define H_FRAME_PARSER_VIDEO_AVS

// /////////////////////////////////////////////////////////////////////
//
//      Include any component headers

#include "avs.h"
#include "frame_parser_video.h"

// /////////////////////////////////////////////////////////////////////////
//
// Locally defined structures
//

#define AVS_PICTURE_DISTANCE_RANGE              256
#define AVS_PICTURE_DISTANCE_HALF_RANGE         (AVS_PICTURE_DISTANCE_RANGE >> 1)


// /////////////////////////////////////////////////////////////////////////
//
// The class definition
//

class FrameParser_VideoAvs_c : public FrameParser_Video_c
{
private:

    // Data

    AvsStreamParameters_t   CopyOfStreamParameters;
    AvsStreamParameters_t  *StreamParameters;
    AvsFrameParameters_t   *FrameParameters;

    int                     LastPanScanHorizontalOffset;
    int                     LastPanScanVerticalOffset;

    bool                    EverSeenRepeatFirstField;
    bool                    LastFirstFieldWasAnI; // Support for self referencing IP field pairs

    unsigned int            PictureDistanceBase;
    unsigned int            LastPictureDistance;

    int                     ImgtrNextP;
    int                     ImgtrLastP;
    int                     ImgtrLastPrevP;

    // Functions

    FrameParserStatus_t     ReadSequenceHeader(                         void );
    FrameParserStatus_t     ReadSequenceDisplayExtensionHeader(         void );
    FrameParserStatus_t     ReadCopyrightExtensionHeader(               void );
    FrameParserStatus_t     ReadCameraParametersExtensionHeader(        void );

    FrameParserStatus_t     ReadPictureHeader(                          unsigned int            PictureStartCode );
    FrameParserStatus_t     ReadPictureDisplayExtensionHeader(          void );
    //FrameParserStatus_t     ReadSliceHeader(                            unsigned int            StartCode,
    //                                                                    unsigned int            Offset );

    FrameParserStatus_t     CommitFrameForDecode(                       void );

    FrameParserStatus_t     CalculateFieldOffsets(                      unsigned int            FirstSliceCodeIndex );

public:

    // Constructor function
    FrameParser_VideoAvs_c( void );
    ~FrameParser_VideoAvs_c( void );

    // Overrides for component base class functions
    FrameParserStatus_t   Reset( void );

    // FrameParser class functions
    FrameParserStatus_t   RegisterOutputBufferRing( Ring_t Ring );

    // Stream specific functions
    FrameParserStatus_t   ReadHeaders( void );
    FrameParserStatus_t   PrepareReferenceFrameList(                            void );
    FrameParserStatus_t   ForPlayUpdateReferenceFrameList(                      void );
    FrameParserStatus_t   RevPlayProcessDecodeStacks(                           void );
    FrameParserStatus_t   RevPlayGeneratePostDecodeParameterSettings(           void );
    FrameParserStatus_t   RevPlayRemoveReferenceFrameFromList(                  void );

};

#endif

