////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <ITr2VolumeObject.h>

#include <AK/SoundEngine/Common/AkTypes.h>

#include <string>

/**
 * @brief A Wwise Spatial Audio room described by a unit box placed in the world.
 *
 * Implements ITr2VolumeObject so that trinity can place the box without knowing anything about
 * acoustics. Everything acoustic (reverb aux bus, priority, transmission loss) lives here as
 * persisted attributes, the same split used between EveChildAudio and AudEmitter.
 */
BLUE_CLASS( AudRoom ) :
	public ITr2VolumeObject
{
public:
	AudRoom( IRoot* lockobj = NULL );
	virtual ~AudRoom();

	EXPOSE_TO_BLUE();

	// ITr2VolumeObject
	void SetTransform( const Matrix& unitBoxToWorld ) override;
	void SetEnabled( bool enabled ) override;
	void Remove() override;

	/// Room identifier. Shares the Wwise game object ID space, see AkRoomID.
	AkUInt64 GetRoomID() const { return m_roomID; }
	/// Name shown in the Wwise profiler.
	const std::string& GetName() const { return m_name; }
	/// Wwise aux bus name used for the room reverb. Empty means no reverb.
	const std::string& GetReverbAuxBus() const { return m_reverbAuxBus; }
	float GetReverbLevel() const { return m_reverbLevel; }
	float GetTransmissionLoss() const { return m_transmissionLoss; }
	float GetPriority() const { return m_priority; }
	float GetAuxSendLevelToSelf() const { return m_auxSendLevelToSelf; }
	bool GetKeepRegistered() const { return m_keepRegistered; }
	/// Active only when both the authored attribute and the shape owner (trinity) say so.
	bool IsEnabled() const { return m_enabled && m_shapeEnabled; }
	bool HasTransform() const { return m_hasTransform; }
	/// False when the box has a zero-length axis; such a room contains nothing and is not sent to Wwise.
	bool HasUsableShape() const { return m_shapeValid; }
	const Matrix& GetUnitBoxToWorld() const { return m_unitBoxToWorld; }

private:
	AkUInt64 m_roomID;

	std::string m_name;
	std::string m_reverbAuxBus;
	float m_reverbLevel;
	float m_transmissionLoss;
	float m_priority;
	float m_auxSendLevelToSelf;
	bool m_keepRegistered;
	/// Authored attribute: the acoustic side of "enabled".
	bool m_enabled;
	/// Set through ITr2VolumeObject::SetEnabled by whoever owns the shape. Starts enabled, never persisted.
	bool m_shapeEnabled;

	Matrix m_unitBoxToWorld;
	bool m_shapeValid;
	bool m_warnedDegenerate;
	bool m_hasTransform;
};

TYPEDEF_BLUECLASS( AudRoom );
