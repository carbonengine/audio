////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <ITr2VolumeObject.h>

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <string>

/**
 * @brief A Wwise Spatial Audio room, placed as a unit box through ITr2VolumeObject.
 */
BLUE_CLASS( AudRoom ) :
	public ITr2VolumeObject,
	public INotify
{
public:
	AudRoom( IRoot* lockobj = NULL );
	virtual ~AudRoom();

	EXPOSE_TO_BLUE();

	// ITr2VolumeObject
	void SetTransform( const Matrix& unitBoxToWorld ) override;
	void SetEnabled( bool enabled ) override;
	void Remove() override;

	// INotify
	bool OnModified( Be::Var* value ) override;

	AkUInt64 GetRoomID() const { return m_roomID; }
	const std::string& GetName() const { return m_name; }
	const std::string& GetReverbAuxBus() const { return m_reverbAuxBus; }
	float GetReverbLevel() const { return m_reverbLevel; }
	float GetTransmissionLoss() const { return m_transmissionLoss; }
	float GetPriority() const { return m_priority; }
	float GetAuxSendLevelToSelf() const { return m_auxSendLevelToSelf; }
	bool GetKeepRegistered() const { return m_keepRegistered; }
	const std::wstring& GetRoomToneEvent() const { return m_roomToneEvent; }
	/// True when the room and its volume object are both enabled.
	bool IsEnabled() const { return m_enabled && m_shapeEnabled; }
	bool HasTransform() const { return m_hasTransform; }
	/// False when the box has a zero-length axis.
	bool HasUsableShape() const { return m_shapeValid; }
	const Matrix& GetUnitBoxToWorld() const { return m_unitBoxToWorld; }
	/// Whether the room is in Wwise.
	bool IsRegistered() const { return m_sentToWwise; }

	/// Returns whether the point is inside the room, in right-handed world space.
	bool ContainsPoint( const Vector3& worldPosition ) const;
	/// Volume of the box, the smaller room wins when priorities are equal.
	float GetVolume() const { return m_volume; }

private:
	friend class AudRoomManager;

	/// Registers with the room manager once it exists.
	bool EnsureRegistered();
	/// Sends the room to Wwise, or removes it when disabled.
	void Sync();
	/// Stores the box. Called by AudRoomManager under its lock.
	void ApplyTransform( const Matrix& unitBoxToWorld );

	AkUInt64 m_roomID;

	std::string m_name;
	std::string m_reverbAuxBus;
	float m_reverbLevel;
	float m_transmissionLoss;
	float m_priority;
	float m_auxSendLevelToSelf;
	bool m_keepRegistered;
	std::wstring m_roomToneEvent;
	bool m_enabled;
	/// Set through ITr2VolumeObject::SetEnabled.
	bool m_shapeEnabled;

	Matrix m_unitBoxToWorld;
	float m_volume;
	bool m_shapeValid;
	bool m_warnedDegenerate;
	bool m_hasTransform;
	bool m_sentToWwise;
	bool m_registeredWithManager;

	/// Last parameters sent to Wwise.
	AkRoomParams m_sentRoomParams;
	std::string m_sentName;

	/// Room tone state, owned by AudRoomManager.
	std::wstring m_postedRoomTone;
	AkPlayingID m_roomTonePlayingID;
	bool m_roomTonePending;
};

TYPEDEF_BLUECLASS( AudRoom );
