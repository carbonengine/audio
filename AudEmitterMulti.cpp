#include "stdafx.h"
#include "AudEmitterMulti.h"
#include "AudManager.h"
#include "Utilities.h"

#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/SoundEngine/Common/AkQueryParameters.h>
#include <AK/SoundEngine/Common/AkTypes.h>

static bool SortDistancePositions( const DistancePosition& p0, const DistancePosition& p1 )
{
	return p0.distance < p1.distance;
}

AudEmitterMulti::AudEmitterMulti( IRoot* lockobj ) : 
	AudGameObjResource( lockobj ), 
	m_currentIndex( 0 )
{
	m_playOnLoad = true;
	m_maximumLocations = 10;
	m_distancePositionVector.resize( m_maximumLocations );
}

AudEmitterMulti::~AudEmitterMulti()
{
	g_audioManager->RemoveMultiEmitterFromList(this);
}

void AudEmitterMulti::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	if( g_audioInitialized )
	{
		AkSoundPosition posLH, posRH;
		posRH.Set( MakeAkVector(pos), MakeAkVector(front), MakeAkVector(top) );
		RH2LH::convertEmitter( &posLH, &posRH );

		AkListenerPosition listener;
		unsigned int default_listener = 0;
		AK::SoundEngine::Query::GetListenerPosition( default_listener, listener );
		AkReal32 x = pow( ( listener.Position().X - pos.x ), 2 );
		AkReal32 y = pow( ( listener.Position().Y - pos.y ), 2 );
		AkReal32 z = pow( ( listener.Position().Z - pos.z ), 2 );
		AkReal32 distanceSquared = x+y+z;

		uint32_t index = m_currentIndex++;
		if( index < m_distancePositionVector.size() )
		{
			m_distancePositionVector[index] = DistancePosition( distanceSquared, posLH );
		}
	}
}

void AudEmitterMulti::ProcessPlacementList() {

	if( m_currentIndex > 0 )
	{
		int count = m_currentIndex;
		if( m_currentIndex > m_maximumLocations )
		{
			std::sort( m_distancePositionVector.begin(), m_distancePositionVector.end(), SortDistancePositions );
			count = m_maximumLocations;
		}

		for( int i = 0; i < count; ++i )
		{
			m_positionVector.push_back(m_distancePositionVector[i].wwisePos);
		}
		AK::SoundEngine::SetMultiplePositions( m_ID, &m_positionVector[0], (AkUInt16) m_positionVector.size(), AK::SoundEngine::MultiPositionType_MultiDirections ); // Multi directions is default
		m_positionVector.clear();
		if( m_currentIndex > m_distancePositionVector.size() )
		{
			m_distancePositionVector.resize( m_currentIndex );
		}
		m_currentIndex = 0;
	}
	else
	{
		Vector3 initpos = Vector3( WISE_INIT_POSITION, WISE_INIT_POSITION, WISE_INIT_POSITION );
		SetPositionHelper( Vector3( 1,0,0 ), Vector3( 0,1,0 ), initpos);
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
	m_distancePositionVector.resize( m_maximumLocations );
}