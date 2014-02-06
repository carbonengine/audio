/* 
	*************************************************************************************

	AudSettings.h

	Author:    Andri Mar
	Created:   March 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Setting store for audio2, used for dynamic runtime settings


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDSETTINGS_H_
#define _AUDSETTINGS_H_

#include "Audio2.h"

#include <map>
#include <string>

BLUE_CLASS( AudSettings ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

    template<typename T> void RegisterSetting( const char* name, T* value )
    {
		Be::VARTYPE type = Be::TypeTraits<T>::VARTYPE_VALUE;
		RegisterSettingHelper( name, value, type, sizeof(T) );
	}

	struct Setting
	{
		Be::VARTYPE	m_type;
		SIZE_T		m_size;
		Be::Var*	m_var;
	};

	typedef std::map<std::string, Setting > SettingMap;
	SettingMap m_map;

	Setting* FindSetting( const char* name )
	{
		SettingMap::iterator it = m_map.find( std::string(name) );
		if( it != m_map.end() )
		{
			return &(*it).second;
		}
		else
		{
			return NULL;
		}
		
	}

	std::string GetReprString()
	{
		std::string repr = "{";
		for( SettingMap::const_iterator it = m_map.begin(); it != m_map.end(); ++it )
		{
			std::string entry = "'" + it->first + "':";
			entry += GetSettingReprString( &it->second );
			entry += ", ";
			repr += entry;
		}
		repr += "}";

		return repr;
	}

	std::string GetSettingReprString( const Setting* s );

private:
	void RegisterSettingHelper( const char* name, void* value, Be::VARTYPE type, size_t size )
	{
		if( type == Be::INVALID )
		{
			CCP_LOGERR( "Type for setting '%s' is not currently supported!", name );
			return;
		}

		Setting s;
		s.m_type = type;
		s.m_size = size;
		s.m_var = (Be::Var*)value;

		m_map[ name ] = s;
    }
};

TYPEDEF_BLUECLASS( AudSettings );

#endif