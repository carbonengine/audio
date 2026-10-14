// Copyright © 2026 CCP ehf.

#include "LabSightlines.h"

#include <BlueRegistration.h>

BLUE_DEFINE_INTERFACE( IEveObstructionQuery );

// Sightlines are on the ground plane: audio space is the lab's x, 0, z.
BLUE_CLASS( LabSightlineQuery ) : public IEveObstructionQuery
{
public:
	LabSightlineQuery( IRoot* lockobj = nullptr ) {}

	EXPOSE_TO_BLUE();

	void SetScene( const AudioLab::LabScene* scene ) { m_scene = scene; }

	bool QuerySightlines( const Vector3& source, const Vector3* targets, unsigned int targetCount, bool* outBlocked ) override
	{
		if( m_scene == nullptr )
		{
			return false;
		}
		for( unsigned int i = 0; i < targetCount; ++i )
		{
			outBlocked[i] = m_scene->IsBlocked( source.x, source.z, targets[i].x, targets[i].z );
		}
		return true;
	}

private:
	const AudioLab::LabScene* m_scene = nullptr;
};

TYPEDEF_BLUECLASS( LabSightlineQuery );
BLUE_DEFINE( LabSightlineQuery );

const Be::ClassInfo* LabSightlineQuery::ExposeToBlue()
{
	EXPOSURE_BEGIN( LabSightlineQuery, "The Audio Lab's sightline query: a sightline is blocked when it crosses a wall of the lab's scene." )
		MAP_INTERFACE( IEveObstructionQuery )
		MAP_INTERFACE( LabSightlineQuery )
	EXPOSURE_END()
}

namespace AudioLab
{
	void RegisterLabClasses()
	{
		BeClasses->RegisterClasses( BlueRegistration::GetClassRegs() );
	}

	BluePtr<IEveObstructionQuery> CreateSightlineQuery( const LabScene& scene )
	{
		BluePtr<LabSightlineQuery> query;
		if( !BeClasses->CreateInstanceFromName( "LabSightlineQuery", BlueInterfaceIID<LabSightlineQuery>(), reinterpret_cast<void**>( &query.p ) ) || !query )
		{
			return BluePtr<IEveObstructionQuery>();
		}
		query->SetScene( &scene );
		return BluePtr<IEveObstructionQuery>( query.p );
	}
}
