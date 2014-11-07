/*
	*************************************************************************************
	Description:   

		An audio entity in game. Can stream one source to multiple positions.
		It is never created directly from python code but created or returned
		from the 
			AudManagaer::GetEmitterForEventName

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
	std::vector<AkSoundPosition> m_positionVector;

};

TYPEDEF_BLUECLASS( AudEmitterMulti );
#endif