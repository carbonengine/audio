#include "stdafx.h"
#include "AudEmitterMulti.h"
#include "AudManager.h"
#include "Utilities.h"

#include "Vector3.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

AudEmitterMulti::AudEmitterMulti( IRoot* lockobj ) : AudGameObjResource( lockobj )
{
	m_playOnLoad = true;
	m_maximumLocations = 30;
}

AudEmitterMulti::~AudEmitterMulti()
{
	g_audioManager->RemoveMultiEmitterFromList(this);
}

void AudEmitterMulti::UpdatePlacement( const Vector3& front, const Vector3& top, const Vector3& pos )
{
	//There is limit on how many posions a sound can have.
	//It is measures in 2kbytes for the message itself which translates
	//to 82 positions max. 
	if( g_audioInitialized && ( m_positionVector.size() < m_maximumLocations ) )
	{
		AkSoundPosition wwisePos;

		//RH 2 LH conversion
		wwisePos.Position = pos;
		wwisePos.Position.Z *= -1.f;
		wwisePos.Orientation = front;
		wwisePos.Orientation.Z *= -1.f;
				
		m_positionVector.push_back( wwisePos );		
	}
}

void AudEmitterMulti::ProcessPlacementList() {

	if( m_positionVector.size() > 0 )
	{
		AK::SoundEngine::SetMultiplePositions( m_ID, &m_positionVector[0], (AkUInt16) m_positionVector.size(), AK::SoundEngine::MultiPositionType_MultiSources ); // Multi directions is default
		m_positionVector.clear();
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
	m_eventID = AK::SoundEngine::GetIDFromString(eventName.c_str());
	m_playEvent = eventName;
	AudGameObjResource::Initialize();
	g_audioManager->AddMultiEmitterToList(this);
}

void AudEmitterMulti::SetMaximumLocations( const unsigned int numberOfLocations )
{
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