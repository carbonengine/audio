#include "stdafx.h"
#include "AudEmitterMulti.h"
#include "AudManager.h"
#include "Utilities.h"

#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkTypes.h>

AudEmitterMulti::AudEmitterMulti( IRoot* lockobj ) : AudGameObjResource( lockobj )
{
	m_playOnLoad = true;
	m_maximumLocations = 10;
}

AudEmitterMulti::~AudEmitterMulti()
{
	g_audioManager->RemoveMultiEmitterFromList(this);
}

void AudEmitterMulti::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	if( g_audioInitialized )
	{
		AkSoundPosition wwisePos;

		//RH 2 LH conversion
		wwisePos.Position = pos;
		wwisePos.Position.Z *= -1.f;
		wwisePos.Orientation = front;
		wwisePos.Orientation.Z *= -1.f;

		AkListenerPosition listener;
		unsigned int default_listener = 0;
		AK::SoundEngine::Query::GetListenerPosition( default_listener, listener );
		AkReal32 x = pow( ( listener.Position.X - pos.X ), 2 );
		AkReal32 y = pow( ( listener.Position.Y - pos.Y ), 2 );
		AkReal32 z = pow( ( listener.Position.Z - pos.Z ), 2 );
		AkReal32 distanceSquared = x+y+z;

		if( m_distancePositionVector.size() > m_maximumLocations )
		{
			DistancePositionVector::iterator max = m_distancePositionVector.begin();
			for( DistancePositionVector::iterator it = max+1; it != m_distancePositionVector.end(); ++it )
			{
				if( it->distance > max->distance )
				{
					max = it;
				}
			}
			if (max->distance > distanceSquared ) 
			{
				m_distancePositionVector.erase( max );
				m_distancePositionVector.push_back( DistancePosition( distanceSquared, wwisePos) );
			}
			
		}
		else
		{
			m_distancePositionVector.push_back( DistancePosition( distanceSquared, wwisePos) );
		}	
	}
}

void AudEmitterMulti::ProcessPlacementList() {

	if( m_positionVector.size() > 0 )
	{
		for( DistancePositionVector::iterator it = m_distancePositionVector.begin(); it != m_distancePositionVector.end(); ++it)
		{
			m_positionVector.push_back(it->wwisePos);
		}
		AK::SoundEngine::SetMultiplePositions( m_ID, &m_positionVector[0], (AkUInt16) m_positionVector.size(), AK::SoundEngine::MultiPositionType_MultiSources ); // Multi directions is default
		m_positionVector.clear();
		m_distancePositionVector.clear();
	}
	else
	{
		Vector3 initpos( WISE_INIT_POSITION, WISE_INIT_POSITION, WISE_INIT_POSITION );
		AkSoundPosition tmp;
		tmp.Orientation = tmp.Position = initpos;
		SetPositionHelper( tmp );
	}
}


void AudEmitterMulti::Initialize( const std::wstring& eventName )
{
	m_name = CW2A( eventName.c_str() );
	m_eventID = AK::SoundEngine::GetIDFromString( eventName.c_str() );
	m_playEvent = eventName;
	AudGameObjResource::Initialize();
	g_audioManager->AddMultiEmitterToList( this );
}

void AudEmitterMulti::SetMaximumLocations( const unsigned int numberOfLocations )
{
	//There is limit on how many positions a sound can have.
	//It is measures in 2kbytes for the message itself which translates
	//to 82 positions max. --AudioKinetic email response
	const unsigned int maximumLocations = 81;
	if( numberOfLocations > maximumLocations )
	{
		m_maximumLocations = maximumLocations;
	}
	else if( numberOfLocations == 0 )
	{
		m_maximumLocations = 1;
	}
	else
	{
		m_maximumLocations = numberOfLocations;
	}
}