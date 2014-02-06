/* 
	*************************************************************************************

	AudParameter.h

	Author:    Andri Mar
	Created:   April 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		TBA


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDPARAMETER_H_
#define _AUDPARAMETER_H_

#include "Audio2.h"
#include <AK/SoundEngine/Common/AkTypes.h>

BLUE_DECLARE( AudGameObjResource );

BLUE_CLASS( AudParameter ) : public INotify
{
public:
	AudParameter( IRoot* lockobj = NULL );
	~AudParameter();

	friend AudGameObjResource;

	EXPOSE_TO_BLUE();

	//--------------------------
	// Blue interfaces
	//--------------------------
	// INotify
	virtual bool OnModified( Be::Var* value );

private:
	AkGameObjectID m_ID;
	std::wstring m_name;
	float m_value;
};

TYPEDEF_BLUECLASS( AudParameter );
BLUE_DECLARE_VECTOR( AudParameter );

#endif