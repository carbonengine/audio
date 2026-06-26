// Copyright © 2009 CCP ehf.

#pragma once
#ifndef _AUDPARAMETER_H_
#define _AUDPARAMETER_H_

BLUE_DECLARE( AudGameObjResource );

BLUE_CLASS( AudParameter ) : public INotify
{
public:
	AudParameter( IRoot* lockobj = NULL );
	~AudParameter();

	friend AudGameObjResource;

	EXPOSE_TO_BLUE();

	// INotify
	virtual bool OnModified( Be::Var* value ) override;

private:
	AkGameObjectID m_ID;
	std::wstring m_name;
	float m_value;
};

TYPEDEF_BLUECLASS( AudParameter );
BLUE_DECLARE_VECTOR( AudParameter );

#endif