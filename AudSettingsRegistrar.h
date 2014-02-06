#pragma once
#ifndef _AUDSETTINGSREGISTRAR_H_
#define _AUDSETTINGSREGISTRAR_H_

#include "AudSettings.h"
#include "AudManager.h"

class AudSettingsRegistrar
{
public:
	template<typename T> AudSettingsRegistrar( const char* name, T* value )
	{
		AudManager::GetSettings().RegisterSetting( name, value );
	}
};

// Use this macro to register settings at filescope!  Like this:
// static s_bingo = 10;
// AUD_REGISTER_SETTING( "Bingo", s_bingo );
#define AUD_REGISTER_SETTING( name_, value_ ) \
	static AudSettingsRegistrar value_##Registration( name_, &value_ )

#endif