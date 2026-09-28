// Copyright © 2026 CCP ehf.

// Test-only module, loaded by the Python tests as _audiotests and never shipped. It stands in for
// destiny's ballpark so the tests can drive CarbonAudio's sightline pass.

#include <BlueExposure.h>
#include <Blue.h>
#include <IEveObstructionQuery.h>

#include <algorithm>

BLUE_DEFINE_INTERFACE( IEveObstructionQuery );

// Gives every emitter the same answer. Set it as AudManager.obstructionQuery.
BLUE_CLASS( FakeObstructionQuery ) : public IEveObstructionQuery
{
public:
	FakeObstructionQuery( IRoot* lockobj = nullptr ) {}

	EXPOSE_TO_BLUE();

	bool QuerySightlines( const Vector3& source, const Vector3* targets, unsigned int targetCount, bool* outBlocked ) override
	{
		if( !m_hasAnswer )
		{
			return false;
		}
		std::fill( outBlocked, outBlocked + targetCount, m_blocked );
		return true;
	}

private:
	bool m_blocked = false;
	bool m_hasAnswer = true;
};

TYPEDEF_BLUECLASS( FakeObstructionQuery );
BLUE_DEFINE( FakeObstructionQuery );

const Be::ClassInfo* FakeObstructionQuery::ExposeToBlue()
{
	EXPOSURE_BEGIN( FakeObstructionQuery, "Stands in for destiny's sightline query in the tests, giving every emitter the same answer." )
		MAP_INTERFACE( IEveObstructionQuery )
		MAP_INTERFACE( FakeObstructionQuery )
		MAP_ATTRIBUTE( "blocked", m_blocked, "Whether every sightline is reported blocked.", Be::READWRITE )
		MAP_ATTRIBUTE( "hasAnswer", m_hasAnswer, "False makes every query return no answer, like destiny with no world loaded.", Be::READWRITE )
	EXPOSURE_END()
}

BLUE_STANDARD_MODULE_INIT( _audiotests )
