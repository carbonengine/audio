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

float AudOcclusion::GetRadiusScale() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetOccluderRadiusScale() : 0.0f;
}

void AudOcclusion::SetRadiusScale( float value )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetOccluderRadiusScale( value );
	}
}

float AudOcclusion::GetLosRecomputeInterval() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetLosRecomputeInterval() : 0.0f;
}

void AudOcclusion::SetLosRecomputeInterval( float value )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetLosRecomputeInterval( value );
	}
}

void AudOcclusion::SetOccluderSphere( uint64_t occluderID, double x, double y, double z, float radius )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetOccluderSphere( occluderID, x, y, z, radius );
	}
}

void AudOcclusion::SetOrigin( double x, double y, double z )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetOccluderOrigin( x, y, z );
	}
}

void AudOcclusion::SetSightlineSource( double x, double y, double z )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->SetSightlineSource( x, y, z );
	}
}

void AudOcclusion::ClearSightlineSource()
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->ClearSightlineSource();
	}
}

bool AudOcclusion::HasSightlineSource() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->HasSightlineSource() : false;
}

void AudOcclusion::RemoveOccluderSphere( uint64_t occluderID )
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->RemoveOccluderSphere( occluderID );
	}
}

void AudOcclusion::ClearOccluderSpheres()
{
	if( AudObstructionOcclusion* occlusion = Occlusion() )
	{
		occlusion->ClearOccluderSpheres();
	}
}

float AudOcclusion::GetEmitterOcclusion( AkGameObjectID emitterID ) const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetEmitterOcclusion( emitterID ) : 0.0f;
}

int AudOcclusion::GetOccluderSphereCount() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetOccluderSphereCount() : 0;
}

std::vector<uint64_t> AudOcclusion::GetBlockingOccluderIDs() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetBlockingOccluderIDs() : std::vector<uint64_t>();
}

uint64_t AudOcclusion::GetBlockingOccluder( AkGameObjectID emitterID ) const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetBlockingOccluder( emitterID ) : 0;
}

double AudOcclusion::GetOriginX() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetOriginX() : 0.0;
}

double AudOcclusion::GetOriginY() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetOriginY() : 0.0;
}

double AudOcclusion::GetOriginZ() const
{
	AudObstructionOcclusion* occlusion = Occlusion();
	return occlusion != nullptr ? occlusion->GetOriginZ() : 0.0;
}
