////////////////////////////////////////////////////////////////////////////////
//
// Creator: Eric Nielsen
// Created: July 2020
// Copyright (c) 2020, CCP Games
//

#pragma once

#include "Audio2.h"

BLUE_CLASS( AudSettings ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

#if _WIN32
	bool m_spatialAudioEnabled = true;
	std::wstring m_audioSrcPath = L"Media";
	std::wstring m_baseSoundBankPath = L"res:/Audio";
	std::wstring m_essentialPath = L"Essential_Media";
	std::wstring m_soundbankLanguage = L"en";
	std::wstring m_stereoAudioDeviceName = L"System_Stereo";
	std::wstring m_spatialAudioDeviceName = L"System"; 
#elif __APPLE__
	bool m_spatialAudioEnabled = false;
	std::string m_audioSrcPath = "Media";
	std::string m_baseSoundBankPath = "res:/Audio";
	std::string m_essentialPath = "Essential_Media";
	std::string m_soundbankLanguage = "en";
	std::string m_stereoAudioDeviceName = "System_Stereo";
	std::string m_spatialAudioDeviceName = "System";
#endif
	std::string m_applicationName = "Eve Online";
};


TYPEDEF_BLUECLASS( AudSettings );
