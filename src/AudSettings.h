////////////////////////////////////////////////////////////////////////////////
//
// Creator: Eric Nielsen
// Created: July 2020
// Copyright (c) 2020, CCP Games
//

#pragma once

#include "Audio2.h"

enum class AudOcclusionMode : int
{
	Off   = -1, // Occlusion disabled, no geometry registered, spatial audio geometry not initialized
	Basic = 0,  // Manual 1-ray line-of-sight check, transmission only
	HQ    = 1   // Wwise Spatial Audio handles diffraction + transmission
};

BLUE_CLASS( AudSettings ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	int m_occlusionMode = static_cast<int>( AudOcclusionMode::Off );

#if _WIN32
	bool m_spatialAudioEnabled = true;
	std::wstring m_audioSrcPath = L"Media";
	std::wstring m_baseSoundBankPath = L"res:/Audio";
	std::wstring m_essentialPath = L"Essential_Media";
	std::wstring m_soundbankLanguage = L"en";
	std::wstring m_stereoAudioDeviceName = L"System_Stereo";
	std::wstring m_spatialAudioDeviceName = L"System"; 
#elif __APPLE__
	bool m_spatialAudioEnabled = true;
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
