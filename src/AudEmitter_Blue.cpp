// Copyright © 2014 CCP ehf.

#include "stdafx.h"

#include "AudEmitter.h"
#include "Vector3.h"

BLUE_DEFINE( AudEmitter );
BLUE_DEFINE_INTERFACE( ITr2AudEmitter );
BLUE_DEFINE_INTERFACE( ITr2AudParentBall );

void AudEmitter::Py__init__( const std::string& name )
{
	m_name = name;
	AudGameObjResource::Initialize();
}

#if BLUE_WITH_PYTHON
unsigned int AudEmitter::SendEventWithPcm( const std::wstring& eventName, const std::wstring& sourceName, PyObject* pcm, int sampleRate, int channels )
{
	Py_buffer view;
	if( PyObject_GetBuffer( pcm, &view, PyBUF_SIMPLE ) != 0 )
	{
		PyErr_Clear();
		CCP_LOGERR( "Wwise event %S was not sent because its PCM is not a bytes-like object.", eventName.c_str() );
		return AK_INVALID_PLAYING_ID;
	}
	const unsigned int playingID = PostEventWithPcm( eventName, sourceName, static_cast<const uint8_t*>( view.buf ), static_cast<size_t>( view.len ), sampleRate, channels );
	PyBuffer_Release( &view );
	return playingID;
}
#endif

const Be::ClassInfo* AudEmitter::ExposeToBlue()
{
	EXPOSURE_BEGIN( AudEmitter, "RAII wrapper for Wwise gameobjects. Python constructor takes in a name of the object as a string" )
		MAP_INTERFACE( IBlueEventListener )
		MAP_INTERFACE( IBluePlacementObserver )
		MAP_INTERFACE( ITr2AudEmitter )
		MAP_INTERFACE( ITr2AudParentBall )
		MAP_INTERFACE( ITr2DebugRenderable )

		MAP_ATTRIBUTE( "rotation", m_authoredRotation, "Authored local audio rotation used by placement providers.\n:jessica-group: Audio Placement", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_PROPERTY_READONLY( "front", GetFront, "Effective front vector sent to Wwise.\n:jessica-hidden: True" )
		MAP_PROPERTY_READONLY( "top", GetTop, "Effective top vector sent to Wwise.\n:jessica-hidden: True" )
		MAP_ATTRIBUTE( "normalizeAttenuationScaling", m_normalizeAttenuationScaling, "Determines whether attenuation scaling for this audio emitter should be normalized\n:jessica-group: Attenuation Normalization", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minNormalizedValue", m_minNormalizedValue, "The minimum number to use for attenuation normalization.\n:jessica-group: Attenuation Normalization", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxNormalizedValue", m_maxNormalizedValue, "The maximum number to use for attenuation normalization.\n:jessica-group: Attenuation Normalization", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minNormalizedScalingFactor", m_minNormalizedScalingFactor, "The minimum scaling factor that can be used to scale attenuation by.\n:jessica-group: Attenuation Normalization", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "maxNormalizedScalingFactor", m_maxNormalizedScalingFactor, "The maximum scaling factor that can be used to scale attenuation by.\n:jessica-group: Attenuation Normalization", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "visualizationRadius", m_visualizationRadius, "Shows a simulated debug attenuation sphere visualization if value is above 0.\n:jessica-group: Debugging", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		(
			"__init__",
			Py__init__,
			1,
			"Takes in a name of the emitter created as a string.\n"
			":param name: Name of Audio Emitter. Defaults to empty string."
		)
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS
		( 
			"SendEvent", 
			PostEvent, 
			2,
		    "Sends an event to the audio system and returns an ID for the playing stream.\n"
			":param eventName: The name of the event to send to the sound engine\n"
			":param bypassPrefix: An optional argument that bypasses the audio emitters defined prefix if set to True."
			":return: The playing ID associated with this event. 0 If sending the event failed.\n"
		)
#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP
		(
			"SendEventWithPcm",
			SendEventWithPcm,
			"Sends an event that plays 16-bit PCM from memory through one of its External Sources.\n"
			"The PCM is copied and held until the event ends, so the caller doesn't need to keep it.\n"
			"The event is never played later: if this emitter is culled, audio is disabled or a SoundBank "
			"the event needs is not loaded, nothing is sent and 0 is returned.\n"
			":param eventName: The event to send. It plays the External Source called sourceName.\n"
			":param sourceName: The External Source's name in the Wwise project.\n"
			":param pcm: A contiguous bytes-like object of interleaved signed 16-bit little-endian samples: a WAV's data chunk, with no header.\n"
			":param sampleRate: Samples per second per channel.\n"
			":param channels: 1 for mono, 2 for stereo.\n"
			":return: The playing ID associated with this event. 0 if sending the event failed.\n"
		)
#endif
		MAP_METHOD_AND_WRAP
		(
			"SetPlacement",
			SetPosition,
			"Update orientation and position of an audio emitter.\n"
			":param front: Vector3 representing the orientation of the object.\n"
			":param top: Vector3 representing the up vector of the object\n"
			":param position: Vector3 representing the world position .\n"
		)
	EXPOSURE_CHAINTO( AudGameObjResource )
}
