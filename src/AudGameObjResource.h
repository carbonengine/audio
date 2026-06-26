// Copyright © 2009 CCP ehf.

#pragma once

#include "Audio2.h"
#include "AudParameter.h"
#include "IPrioritizedObject.h"

struct Vector3;

// ------------------------------------------------------------------------
// Description:
//   Deals with all things related to a Wwise game object such as playing 
//	 sounds, stopping sounds, registering/unregistering, and setting 3d positions.
//   It is not meant to be exposed through Blue and rather to be built off of as 
//   a base class.
// SeeAlso:
//   AudEmitter, AudUIPlayer, AudListener 
// ------------------------------------------------------------------------
BLUE_CLASS( AudGameObjResource ) : public IInitialize
	                             , public IListNotify
								 , public INotify
	                             , public IPrioritizedObject
{
public:
	AudGameObjResource( IRoot* lockobj = NULL );
	virtual ~AudGameObjResource();

	EXPOSE_TO_BLUE();

	// IInitialized
	bool Initialize() override;
	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList ) override;
	// INotify
	virtual bool OnModified( Be::Var* value ) override;

	// AudGameObjResource
	void Initialize( const std::string& name, const std::wstring& prefix, const Vector3& position );
	// Send an event to be played on this game object.
	unsigned int PostEvent( const std::wstring& name, bool bypassPrefix = false, AkUInt32 additionalFlags = 0x0000 );
	// Register a game object in Wwise.
	virtual void RegisterWwiseObject();
	// Unregister a game object in Wwise.
	void UnregisterWwiseObject();
	// Seek on an event by using a percentage of its duration.
	bool SeekOnEventPercent( const unsigned int playingID, const float percentToSeek );
	//Seek on an event using milliseconds.
	bool SeekOnEventMs( const unsigned int playingID, const unsigned int msToSeek );
	// Set the attenuation scaling factor for all playing events on this game object.
	virtual bool SetAttenuationScalingFactor( float value );
	// Set a Wwise RTPC on this game object.
	bool SetRTPC( const std::wstring& rtpcName, float rtpcValue );
	// Set a Wwise switch on this game object.
	bool SetSwitch( const std::wstring& switchGroup, const std::wstring& switchState );
	// Stop all instances of the given event playing on this game object.
	bool StopEvent( const std::wstring& eventName, uint32_t fadeOutDuration = 1000 ); 
	// Stop the given playingID if it is playing on this game object.
	virtual void StopSound( AkPlayingID playingID, uint32_t fadeOutDuration = 1000 ); 
	// Break the given playingID if it is playing on this game object. Breaking a sound will stop any loops but allow one shots to finish playing.
	void BreakSound( AkPlayingID playingID, uint32_t fadeOutDuration = 1000 );
	// Stop all sounds playing on this audio game object.
	void StopAll(); 
	// Wake up a game object from being culled and put it back in Wwises purview.
	void Wake() override;
	// Cull a game object removing it from Wwises purview and doing minimal calcuations on our side.
	void Cull() override;
	// Calculate the weight of this game object in the culling system based on a number of factors.
	void CalculateCullingWeight(std::chrono::steady_clock::time_point now) override;
	// Whether this game object is culled or not.
	bool IsCulled() const override; 
	// Get the cumulative weight of this game object in the culling system.
	float GetCullingWeight() const override; 
	// Get the ID of this game object.
	AkGameObjectID GetID() const override;
	std::wstring GetEventName();
	// Get the current position of this game object.
	Vector3 GetPosition() const override;
	// Set the squared length of the distance that this game object sits from the listener.
	void SetDistanceSqFromListener(float distanceSq) override;
	// Set the Wwise event to be sent to this game object when it is initialized.
	void SetEventName(const std::wstring& eventName);
	// Mute this game object so it doesn't play sounds. 
	void Mute();
	// Unmute this game object so it can play sounds again.
	void Unmute();
	// Whether or not this game object is currently muted.
	bool IsMuted();


	// Callbacks
	virtual void EventFinishedCallback( AkEventCallbackInfo* cbInfo );

	// Functions to use for testing/debugging
	std::map<unsigned int, std::wstring> GetPlayingEvents();
	// Get the switches currently set on this game object.
	const std::map<std::wstring, std::wstring>& GetSwitches() const;
	// Force this game object to change it's culling state from awake to culled or vice versa.  You must call ReleaseForcedCullingState to get it to return to normal.
	void ForceCullingStateChange();
	// Return this game object to be controlled by the sound prioritization system. 
	void ReleaseForcedCullingState();

protected:
	enum ActionTypes
	{
		Stop = AkActionOnEventType_Stop,
		Break = AkActionOnEventType_Break,
	};
	AudGameObjResource( AkGameObjectID gameObjID, IRoot* lockobj = NULL );
	// Convert a Trinity RH vector to a Wwise LH vector and set the position for this game object in Wwise.
	virtual int SetPositionHelper( const Vector3& front, const Vector3& top, const Vector3& position );
	// Prepend an event prefix if one exists on the given event.
	std::wstring PrepareEvent( const std::wstring& event, bool bypassPrefix );
	// Propagate any wwise callbacks received for this game object.
	static void PropagateWwiseCallback( AkCallbackType in_eType, AkEventCallbackInfo* in_pEventInfo, void* in_pCallbackInfo, void* in_pCookie );
	// Execute the given action on a playing ID if it is playing on this game object.
	void ExecuteActionOnPlayingID( const AkPlayingID playingID, const ActionTypes action, uint32_t fadeOutDuration = 1000 );
	// Calculates and updates sound prioritization attributes that are determined by currently playing events or events that will play on wake.
	void UpdateEventSoundPrioritizationAttributes();
	// Update the max attenuation radius of this game object if the given event's radius is larger than the current value.
	void UpdateMaxAttenuationRadiusForEvent( const std::wstring& eventName );
	// Get the max attenuation radius. The scaling factor of this game object will also be taken into account.
	float GetMaxAttenuationRadius() const;

	AkGameObjectID m_ID;
	std::string m_name;
	std::wstring m_eventPrefix;
	// The Wwise event name to you want to play on this game object when it is first loaded.
	std::wstring m_eventName;
	PAudParameterVector m_parameters;
	Vector3 m_position;
	// Whether this game object is currently registered with Wwise.
	bool m_gameObjRegistered;
	// Whether this game object is culled or not.
	bool m_culled;
	// Whether this game object is visible to the camera.
	bool m_isVisible;
	// Signals whether the listener is in range of sounds playing on this game object.
	bool m_listenerInRange;
	// Signals whether this game object is active (a.k.a. has had any events be sent to it).
	bool m_isUsed;
	// Signals whether a 2d sound is being played on this game object.
	bool m_playing2DSound;
	// Signals that this game object is playing a sound considered vital 
	bool m_playingVitalSound;
	// Signals whether the current culling state of this game object (e.g. culled or awake) should be forced to stay this way. Used for debug purposes.
	bool m_forceCullingState;
	// Signals whether this audio emitter is currently muted or not.
	bool m_muted;
	// The distance of this game object from the listener.
	float m_distanceSqFromListener;
	// Any additional weight you want this game object to have in the culling system. Can be from 0.0 to infinity. The higher the value the less likely it is to be culled.
	float m_additionalCullingWeight;
	// The cumulative culling weight of this game object. The lower it is the more likely it is to not be culled.
	float m_cumulativeWeight;
	// The Wwise scaling factor for this game object.
	float m_scalingFactor;
	// The max attenuation radius this game object has.
	float m_maxAttenuationRadiusSq;

	bool m_hasReceivedPosition;
	// Events waiting to play on this game object when it wakes up from being culled.
	std::set<std::wstring> m_eventsOnWake;
	// Currently playing events on this game object.
	std::map<unsigned int, std::wstring> m_playingEvents;
	// Keeps track of RTPCs sent to this game object.
	std::map<std::wstring, float> m_rtpcValues;
	// Keeps track of switches sent to this game object.
	std::map<std::wstring, std::wstring> m_switchValues;
	// A one shot event sent to this game object while it was culled. 
	std::pair<std::chrono::steady_clock::time_point, std::wstring> m_waitingOneShotInRange;

	// A mutex to be used when working with m_playingEvents and m_eventsOnWake as they are accessed in different threads.
	CcpMutex m_mutex;
};

TYPEDEF_BLUECLASS( AudGameObjResource );
