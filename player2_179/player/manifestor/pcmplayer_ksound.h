////////////////////////////////////////////////////////////////////////////
/// COPYRIGHT (C) STMicroelectronics 2007
///
/// Source file name : pcmplayer_ksound.h
/// Author :           Daniel
///
/// Concrete definition of a PCM player implemented using the ksound ALSA subset.
///
///
/// Date        Modification                                    Name
/// ----        ------------                                    --------
/// 27-Jun-07   Created                                         Daniel
///
////////////////////////////////////////////////////////////////////////////

#ifndef H_PCMPLAYER_KSOUND_CLASS
#define H_PCMPLAYER_KSOUND_CLASS

#include "pcmplayer.h"
#include "ksound.h"

#define Q11_5_UNITY 32
////////////////////////////////////////////////////////////////////////////
///
/// PCM player implemented using the ksound ALSA subset.
///
class PcmPlayer_Ksound_c : public PcmPlayer_c
{
private:

	ksnd_pcm_t *SoundcardHandle;
	
	const snd_pcm_channel_area_t *SoundcardMappedBuffer;
	snd_pcm_uframes_t SoundcardMappedOffset;
	snd_pcm_uframes_t SoundcardMappedSamples;
	
	unsigned long long TimeOfNextCommit;

	int BufferType;		//this will hold identifier for relayfs

	/* this lot for FSynth control access */
	snd_ctl_elem_id_t id;
	snd_ctl_elem_value_t control;
	snd_kcontrol_t *elem;
	int fsynth_adjust;
	
	/* this controls are (optionally) used to manipulate iec60958 channel status bits */
	snd_ctl_elem_id_t iec958_id;
	snd_ctl_elem_value_t iec958_control;
	snd_kcontrol_t *iec958_elem;

	/* these controls are (optionally) used to manipulate iec60958 validity bits */
	snd_ctl_elem_id_t iec958_val_id;
	snd_ctl_elem_value_t iec958_val_control;
	snd_kcontrol_t *iec958_val_elem;

	int SPDIFmode;
	/// The factor by which the number of samples will be increased (in Q11.5 fixed point).
	unsigned int ResamplingFactor_x32;
	
	char Identity[24];

public:

	PcmPlayer_Ksound_c(unsigned int Soundcard, unsigned int Substream, unsigned int dev_num, unsigned int SPDIFmode);
	~PcmPlayer_Ksound_c();

	PlayerStatus_t MapSamples(unsigned int SampleCount, bool NonBlock, void **MappedSamplesPP);
	PlayerStatus_t CommitMappedSamples();

	PlayerStatus_t GetTimeOfNextCommit(unsigned long long *TimeP);

	PlayerStatus_t SetParameters(PcmPlayerSurfaceParameters_t *ParamsP);
	PlayerStatus_t SetOutputRateAdjustment(int Rate, int *ActualRateP);
	PlayerStatus_t SetIec60958StatusBits(struct snd_pseudo_mixer_aes_iec958 *Iec958Status);
	PlayerStatus_t SetIec61937StatusBits(struct snd_pseudo_mixer_aes_iec958 *Iec958Status);
	
	unsigned int SamplesToBytes(unsigned int SampleCount);
	unsigned int BytesToSamples(unsigned int ByteCount);
	
private:
    
    	PlayerStatus_t DeployUnderrunRecovery();
};

#endif // H_PCMPLAYER_KSOUND_CLASS
