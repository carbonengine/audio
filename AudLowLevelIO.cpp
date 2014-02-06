#include "stdafx.h"

#include "AudLowLevelIO.h"

AudLowLevelIO::AudLowLevelIO( IRoot *lockobj )
{}

AudLowLevelIO::~AudLowLevelIO()
{}

void AudLowLevelIO::Initialize( const std::wstring& path, const std::wstring& lang )
{
	SetBasePath( path.c_str() );
	if( lang.empty() )
	{
		SetLangSpecificDirName( L"English(US)/" );
	}
	else
	{
		SetLangSpecificDirName( lang.c_str() );
	}
}

std::wstring AudLowLevelIO::GetSelectedLangugePath( )
{
	return m_szLangSpecificDirName;
}