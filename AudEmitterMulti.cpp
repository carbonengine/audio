#include "stdafx.h"
#include "AudEmitterMulti.h"
#include "AudManager.h"

#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudEmitterMulti::AudEmitterMulti( IRoot* lockobj ) : AudGameObjResource( lockobj )
{
}

AudEmitterMulti::~AudEmitterMulti()
{
}

void AudEmitterMulti::UpdatePlacements( const PositionDescriptionVector& positions )
{
	if( g_audioInitialized )
	{
		// Copying the AKSoundPosition value from AudPosition since AudPosition
		// has more datamembers and therefore cannot be used directly as a
		// parameter in SetMultiplePositions since it uses raw memcpy to
		// move the data to its own local store.
		std::vector<AkSoundPosition> posVec;

		for ( PositionDescriptionVector::const_iterator itr = positions.begin(); itr != positions.end(); ++itr )
		{
			AkSoundPosition soundPos;
			soundPos.Position.X = (*itr).pos_x;
			soundPos.Position.Y = (*itr).pos_y;
			soundPos.Position.Z = (*itr).pos_z;
			soundPos.Orientation.X = (*itr).front_x;
			soundPos.Orientation.Y = (*itr).front_y;
			soundPos.Orientation.Z = (*itr).front_z;
			
			posVec.push_back( soundPos );
		}

		AK::SoundEngine::SetMultiplePositions( m_ID, &posVec[0], (AkUInt16) posVec.size() ); // Multi directions is default
	}
}
