// Copyright © 2026 CCP ehf.

// Test-only module, loaded by the Python tests as _audiotests and never shipped. It stands in for
// destiny's ballpark so the tests can drive CarbonAudio's sightline pass.

#include <BlueExposure.h>
#include <Blue.h>
#include <IEveObstructionQuery2.h>

#include <algorithm>

BLUE_DEFINE_INTERFACE( IEveObstructionQuery2 );

// Gives every emitter the same answer, except emitters mounted on clearParent, which are always clear the way
// destiny skips an emitter's own ball. Set it as AudManager.obstructionQuery.
BLUE_CLASS( FakeObstructionQuery ) : public IEveObstructionQuery2
{
public:
	FakeObstructionQuery( IRoot* lockobj = nullptr ) {}

	EXPOSE_TO_BLUE();

	bool QuerySightlines( const Vector3& source, const Vector3* targets, IRoot* const* targetParents, unsigned int targetCount, bool* outBlocked ) override
	{
		if( !m_hasAnswer )
		{
			return false;
		}
		for( unsigned int i = 0; i < targetCount; ++i )
		{
			const bool onClearParent = m_clearParent != nullptr && targetParents[i] == m_clearParent;
			outBlocked[i] = m_blocked && !onClearParent;
		}
		return true;
	}

private:
	bool m_blocked = false;
	bool m_hasAnswer = true;
	IRootPtr m_clearParent;
};

TYPEDEF_BLUECLASS( FakeObstructionQuery );
BLUE_DEFINE( FakeObstructionQuery );

const Be::ClassInfo* FakeObstructionQuery::ExposeToBlue()
{
	EXPOSURE_BEGIN( FakeObstructionQuery, "Stands in for destiny's sightline query in the tests, giving every emitter the same answer." )
		MAP_INTERFACE( IEveObstructionQuery2 )
		MAP_INTERFACE( FakeObstructionQuery )
		MAP_ATTRIBUTE( "blocked", m_blocked, "Whether every sightline is reported blocked.", Be::READWRITE )
		MAP_ATTRIBUTE( "hasAnswer", m_hasAnswer, "False makes every query return no answer, like destiny with no world loaded.", Be::READWRITE )
		MAP_ATTRIBUTE( "clearParent", m_clearParent, "Emitters mounted on this object are always reported clear, like destiny skipping an emitter's own ball.", Be::READWRITE )
	EXPOSURE_END()
}

BLUE_STANDARD_MODULE_INIT( _audiotests )
