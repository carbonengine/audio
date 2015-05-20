#include "stdafx.h"
#include "AudEmitterDoppler.h"
#include "AudManager.h"
#include "Vector3.h"
#include "Utilities.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>

AudEmitterDoppler::AudEmitterDoppler( IRoot* lockobj ) : AudEmitter( lockobj )
{
	m_lastKnowDistanceSq = 0;
}

AudEmitterDoppler::~AudEmitterDoppler()
{
	AK::SoundEngine::StopAll( m_ID );
}

void AudEmitterDoppler::UpdatePlacement(const Vector3& front, const Vector3& top, const Vector3& pos )
{
	if (g_audioInitialized)
	{
		AkSoundPosition tmp;
		AkSoundPosition posLH;
		tmp.Orientation = front;
		tmp.Position = pos;
		RH2LH::convertEmitter( &posLH, &tmp );
		if( g_audioManager->m_useDoppler )
		{
			AkListenerPosition listener;
			AkUInt32 defaultListener = 0;
			AK::SoundEngine::Query::GetListenerPosition( defaultListener, listener );

			AkReal32 x = pow( ( listener.Position.X - posLH.Position.X ), 2 );
			AkReal32 y = pow( ( listener.Position.Y - posLH.Position.Y ), 2 );
			AkReal32 z = pow( ( listener.Position.Z - posLH.Position.Z ), 2 );
			AkReal32 distanceSq = x+y+z;

			AkReal32 deltaDistance = m_lastKnowDistanceSq - distanceSq;
			m_lastKnowDistanceSq = distanceSq;
			
			AK::SoundEngine::SetRTPCValue( m_rtpcID, deltaDistance, m_ID, m_dopplerChangeDuration );
			m_rtpcIsDirty = true;
		}
		else if( m_rtpcIsDirty )
		{
			AK::SoundEngine::SetRTPCValue( m_rtpcID, 0, m_ID, m_dopplerChangeDuration );
			m_rtpcIsDirty = false;
		}
		AK::SoundEngine::SetPosition( m_ID, posLH );
	}
}