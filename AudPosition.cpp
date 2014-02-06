#include "stdafx.h"
#include "AudPosition.h"
#include "AudEmitterMulti.h"

#include "Vector3.h"

AudPosition::AudPosition( IRoot *lockobj )
{}

AudPosition::~AudPosition()
{
}

void AudPosition::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	m_value.Orientation = front;
	m_value.Position = pos;
}

bool AudPosition::OnModified( Be::Var* value )
{
	return true;
}