#include "StdAfx.h"
#include "AudMp3Info.h"

BLUE_DEFINE_NONEXPOSED( AudMp3Info );

const Be::ClassInfo* AudMp3Info::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudMp3Info, "" )
		MAP_INTERFACE( IRoot )

		MAP_ATTRIBUTE( "numChannels", uNumChannels, "Number of channels, usually 2(stereo).", Be::READ )
		MAP_ATTRIBUTE( "sampleRate", uSampleRate, "Number of samples taken per second. Usually 44.1 kHz(CD quality)", Be::READ )
		MAP_ATTRIBUTE( "duration", msDuration, "Duration of file in milliseconds.", Be::READ )
		MAP_ATTRIBUTE( "bitRate", uBitRate, "Bitrate in kbps.", Be::READ )
		MAP_ATTRIBUTE( "firstFrameOffset", uFirstFrameOffset, "Offset of first MPEG frame in file", Be::READ )
		MAP_ATTRIBUTE( "mpegStreamSize", uMPEGStreamSize, "Size of MPEG stream. Not including tags!", Be::READ )
	EXPOSURE_END()
}