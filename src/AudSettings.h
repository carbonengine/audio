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
	std::wstring m_baseSoundBankPath = L"res:/Audio";
	std::wstring m_essentialPath = L"Essential_Media";
	std::wstring m_soundbankLanguage = L"en";
#elif __APPLE__
	std::string m_baseSoundBankPath = "res:/Audio";
	std::string m_essentialPath = "Essential_Media";
	std::string m_soundbankLanguage = "en";
#endif
	std::string m_applicationName = "Eve Online";
};


TYPEDEF_BLUECLASS( AudSettings );
