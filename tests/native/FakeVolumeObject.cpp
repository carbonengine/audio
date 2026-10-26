// Copyright © 2026 CCP ehf.

// Test-only stand-in for trinity's EveVolumeObject, so the tests can place AudRoom and AudPortal boxes.

#include <BlueExposure.h>
#include <Blue.h>
#include <ITr2BoxVolume.h>

BLUE_DEFINE_INTERFACE( ITr2BoxVolume );
BLUE_DECLARE_INTERFACE( ITr2BoxVolume );

// Places an axis-aligned box on its consumer. A negative size turns that axis round.
BLUE_CLASS( FakeVolumeObject ) : public IRoot
{
public:
	FakeVolumeObject( IRoot* lockobj = nullptr ) {}

	EXPOSE_TO_BLUE();

	void Place( const Vector3& center, const Vector3& size )
	{
		if( !m_consumer )
		{
			return;
		}

		Matrix unitBoxToWorld = {};
		unitBoxToWorld._11 = size.x;
		unitBoxToWorld._22 = size.y;
		unitBoxToWorld._33 = size.z;
		unitBoxToWorld._41 = center.x;
		unitBoxToWorld._42 = center.y;
		unitBoxToWorld._43 = center.z;
		unitBoxToWorld._44 = 1.0f;
		m_consumer->SetTransform( unitBoxToWorld );
	}

private:
	ITr2BoxVolumePtr m_consumer;
};

TYPEDEF_BLUECLASS( FakeVolumeObject );
BLUE_DEFINE( FakeVolumeObject );

const Be::ClassInfo* FakeVolumeObject::ExposeToBlue()
{
	EXPOSURE_BEGIN( FakeVolumeObject, "Stands in for trinity's EveVolumeObject in the tests, placing a box on an AudRoom or AudPortal." )
		MAP_INTERFACE( FakeVolumeObject )
		MAP_ATTRIBUTE( "consumer", m_consumer, "The AudRoom or AudPortal that receives the box.", Be::READWRITE )
		MAP_METHOD_AND_WRAP
		(
			"Place",
			Place,
			"Places the box on the consumer.\n"
			":param center: Centre of the box.\n"
			":param size: Width, height and depth of the box."
		)
	EXPOSURE_END()
}
