#include "stdafx.h"
#include "AudWorld.h"
#include "AudWorldFrame.h"

#include <atomic>

namespace
{
	std::atomic<uint64_t> s_nextSource{ 1 };
}

AudWorld::AudWorld( IRoot* lockobj ) :
	m_source( s_nextSource++ )
{}

AudWorld::~AudWorld()
{
	AudWorldFrame::Get().ReleaseSource( m_source );
}

void AudWorld::SetSceneOrigin( double x, double y, double z )
{
	AkVector64 origin;
	origin.X = x;
	origin.Y = y;
	origin.Z = z;
	AudWorldFrame::Get().SetSceneOrigin( origin, m_source );
}
