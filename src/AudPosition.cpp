// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudPosition.h"

#include "Vector3.h"

AudPosition::AudPosition( IRoot* lockobj ) :
	m_value( AkSoundPosition() )
{}

AudPosition::~AudPosition()
{
}

void AudPosition::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_value.Set( MakeAkVector(pos), MakeAkVector(front), MakeAkVector(top) );
}

bool AudPosition::OnModified( Be::Var* value )
{
	return true;
}