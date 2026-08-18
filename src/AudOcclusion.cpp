// Copyright © 2026 CCP ehf.

#include "stdafx.h"
#include "AudOcclusion.h"

#include "AudManager.h"
#include "AudObstructionOcclusion.h"

AudOcclusion::AudOcclusion( IRoot* lockobj )
{
}

AudObstructionOcclusion* AudOcclusion::Occlusion()
{
	return g_audioManager != nullptr ? g_audioManager->GetObstructionOcclusion() : nullptr;
}

bool AudOcclusion::GetEnabled() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->IsEnabled() : false;
}

void AudOcclusion::SetEnabled( bool value )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetEnabled( value );
	}
}

float AudOcclusion::GetFadeRate() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetFadeRate() : 0.0f;
}

void AudOcclusion::SetFadeRate( float value )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetFadeRate( value );
	}
}

float AudOcclusion::GetBlockedOcclusion() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetBlockedOcclusion() : 0.0f;
}

void AudOcclusion::SetBlockedOcclusion( float value )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetBlockedOcclusion( value );
	}
}

void AudOcclusion::SetEmitterBlocked( AkGameObjectID emitterID, bool blocked )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetEmitterBlocked( emitterID, blocked );
	}
}

void AudOcclusion::ClearAllBlocked()
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->ClearAll();
	}
}

float AudOcclusion::GetEmitterOcclusion( AkGameObjectID emitterID ) const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetEmitterOcclusion( emitterID ) : 0.0f;
}
