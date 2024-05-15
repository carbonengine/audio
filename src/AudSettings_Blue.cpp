#include "stdafx.h"
#include "AudSettings.h"

BLUE_DEFINE( AudSettings );

const Be::ClassInfo* AudSettings::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudSettings, "Encapsulates settings for Audio" )
		MAP_INTERFACE(AudSettings)

		MAP_ATTRIBUTE("baseSoundbankPath", m_baseSoundBankPath, "The filepath where soundbanks should be found.", Be::READWRITE )
		MAP_ATTRIBUTE("soundbankLanguage", m_soundbankLanguage, "The language to be used in the soundbank.", Be::READWRITE )
		MAP_ATTRIBUTE("applicationName", m_applicationName, "The name that will appear in Wwise debugging when connecting to audio2.", Be::READWRITE )
		MAP_ATTRIBUTE("essentialPath", m_essentialPath, "The path for essential media.", Be::READWRITE )
	EXPOSURE_END()
}
