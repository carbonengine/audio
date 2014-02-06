#pragma once
#ifndef AudEventKey_h
#define AudEventKey_h


#include <string>

BLUE_CLASS( AudEventKey ):
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

    AudEventKey( IRoot* lockobj = NULL );
	~AudEventKey();

	float m_time;
	std::wstring m_value;
};
BLUE_DECLARE_VECTOR( AudEventKey );

TYPEDEF_BLUECLASS( AudEventKey );
#endif //AudEventKey_h
