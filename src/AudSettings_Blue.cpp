#include "stdafx.h"
#include "AudSettings.h"

BLUE_DEFINE( AudSettings );

const Be::ClassInfo* AudSettings::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudSettings, "Encapsulates settings for Audio" )
		MAP_INTERFACE(AudSettings)

		MAP_ATTRIBUTE(
			"baseSoundbankPath", 
			m_baseSoundBankPath, 
			"The filepath where soundbanks should be found.", 
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"soundbankLanguage", 
			m_soundbankLanguage, 
			"The language to be used in the soundbank.", 
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"applicationName", 
			m_applicationName, 
			"The name that will appear in Wwise debugging when connecting to audio2.", 
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"stereoAudioDeviceName", 
			m_stereoAudioDeviceName, 
			"The name of the Wwise audio device to be used if the user wants to disable spatial audio and return to stereo. "
			"This name must match the name of an audio device in your Wwise project that has 'Allow 3D Audio' toggled off. "
			"This works even if the user has a spatial audio endpoint enabled (e.g. Dolby Atmos). Only set this if your "
			"project has a different stereo audio device name than the default.",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"spatialAudioDeviceName", 
			m_spatialAudioDeviceName, 
			"The name of the Wwise audio device to be used if the user has spatial audio enabled. This name must match the name"
			"of an audio device in your Wwise project that has 'Allow 3D Audio' toggled on. Only set this if your project has "
			"a different spatial audio device name than the default. Note: spatial audio only works on Windows.",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"spatialAudioEnabled", 
			m_spatialAudioEnabled, 
			"Whether spatial (or 3D) audio should be activated when initializing Carbon Audio. This requires that your Wwise project has an audio device"
			"whose name matches the spatialAudioDeviceName property in AudSettings and that it has 'Allow 3D Audio' toggled on. "
			"If the user's system does not have a 3d audio endpoint enabled (e.g. Dolby Atmos) then this will not take effect until they activate it."
			"Note: spatial audio only works on Windows.",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE("essentialPath", m_essentialPath, "The path for essential media.", Be::READWRITE )
		MAP_ATTRIBUTE(
			"occlusionMode",
			m_occlusionMode,
			"Controls spatial audio occlusion. 0 = Off (no geometry, no occlusion), "
			"1 = On ( Wwise Spatial Audio Diffraction + transmission ).",
			Be::READWRITE
		)
	EXPOSURE_END()
}
