////////////////////////////////////////////////////////////
//
//    Creator:   Eric Nielsen 
//    Created:   February 2019
//    Copyright: CCP 2019
//

#pragma once
#include <AK/SoundEngine/Common/AkTypes.h>
#include "trinity/Include/ITr2AudioEmitter.h"

#include "AudGameObjResource.h"
#include "AudPosition.h"

// --------------------------------------------------------------------------------
// Description:
//   A simple audio emitter. Can be used to simply fire 
//   an audio event at a given position.
// --------------------------------------------------------------------------------
BLUE_CLASS( AudioEmitter ) : public ITr2AudioEmitter,
							 public AudGameObjResource
{
public:
	AudioEmitter( IRoot* lockobj = NULL );
	virtual ~AudioEmitter();

	EXPOSE_TO_BLUE();

	// Will be called on python init and creates a game object for the sound engine
	void Py__init__();
	// Sends m_eventName to the sound engine
	void SendEvent();
	// Sets the transform of the game object for the sound engine
	void SetTransform( const Matrix& worldTransform );
protected:
	std::wstring m_eventName;
private:
	AkSoundPosition m_position;
};

TYPEDEF_BLUECLASS( AudioEmitter );
