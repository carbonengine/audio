#include "stdafx.h"
#include "AudWorld.h"
#include "AudWorldFrame.h"

AudWorld::AudWorld( IRoot* lockobj )
{}

AudWorld::~AudWorld()
{}

void AudWorld::SetSceneOrigin( double x, double y, double z )
{
	AkVector64 origin;
	origin.X = x;
	origin.Y = y;
	origin.Z = z;
	AudWorldFrame::Get().SetSceneOrigin( origin );
}
