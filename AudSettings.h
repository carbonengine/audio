////////////////////////////////////////////////////////////////////////////////
//
// Creator: Eric Nielsen 
// Created: July 2020
// Copyright: CCP 2020
//

#pragma once

#include "Audio2.h"

BLUE_CLASS( AudSettings ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	std::wstring m_baseSoundBankPath = L"res/Audio";
	std::wstring m_soundbankLanguage = L"en";
	std::string m_applicationName = "Eve Online";
};


TYPEDEF_BLUECLASS( AudSettings );
