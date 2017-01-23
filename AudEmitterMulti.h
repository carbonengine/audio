/*
	*************************************************************************************
	Description:   

		An audio entity in game. Can stream one source to multiple positions.
		It is never created directly from python code but created or returned
		from the; 
			AudManagaer::GetEmitterForEventName

		Its using the;
			AK::SoundEngine::MultiPositionType_MultiSources

		implementation which means that if too many locations are allowed then
		there is a danger of volume stacking is all locations are really near
		or if the attenuation settings is really big for the given event.

		If less then m_maximumLocations objects send their position they will
		all be added to the list and then processed on the next frame and the
		locations of the multiemitter then updated. If more then m_maximumLocations
		are used, on each new add it will find the biggest distance in the list
			O(n-1) n=m_maximumLocations
		and if depending on the new location, either deleted the biggest distance
		and replace it with the new location, or simply just discard it.

	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

#pragma once
#ifndef _AUDEMITTERMULTI_H_
#define _AUDEMITTERMULTI_H_

#include "AudGameObjResource.h"
#include "AudPosition.h"

// Blue headers specific to this file
#include <blue/include/IBluePlacementObserver.h>
#include <blue/include/IBlueEventListener.h>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <vector>

struct Vector3;
struct DistancePosition {
	AkReal32 distance;
	AkSoundPosition wwisePos;
	DistancePosition( AkReal32 dist, AkSoundPosition pos ) : distance( dist ), wwisePos( pos ) {}
	DistancePosition() : distance( 0.f ) {}
};
typedef std::vector<DistancePosition> DistancePositionVector;
typedef std::vector<AkSoundPosition> PositionVector;

BLUE_CLASS( AudEmitterMulti ) :	public IBluePlacementObserver
							  , public AudGameObjResource
{
public:
	AudEmitterMulti( IRoot* lockobj = NULL );
	virtual ~AudEmitterMulti();

	EXPOSE_TO_BLUE();
	
	void Initialize( const std::wstring& eventName );
	virtual void UpdatePlacement(  const Vector3& front, const Vector3& top, const Vector3& pos );
	void ProcessPlacementList();
	void SetMaximumLocations( const unsigned int numberOfLocations );

	AkUniqueID m_eventID;
	unsigned int m_maximumLocations;

private:
	CcpAtomic<uint32_t> m_currentIndex;
	DistancePositionVector m_distancePositionVector;
	PositionVector m_positionVector;
};

TYPEDEF_BLUECLASS( AudEmitterMulti );
#endif