/**
 * @file AudRoom.h
 * @author Phevos Rinis
 * @date Sep 2026
 * @copyright © 2026 Fenris Creations ehf.
 */

#pragma once

#include <ITr2BoxVolume.h>

#include "AudUnitBox.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <string>

/**
 * @brief A Wwise Spatial Audio room, placed as a unit box through ITr2BoxVolume.
 */
BLUE_CLASS( AudRoom ) :
	public ITr2BoxVolume,
	public INotify
{
public:
	AudRoom( IRoot* lockobj = NULL );
	virtual ~AudRoom();

	EXPOSE_TO_BLUE();

	/// Stores the room's box and sends the room to Wwise.
	void SetTransform( const Matrix& unitBoxToWorld ) override;
	/// Called when the volume object is enabled or disabled.
	void SetEnabled( bool enabled ) override;
	/// Clears the box and removes the room from Wwise.
	void Remove() override;

	/// Called by Blue when an attribute changes.
	bool OnModified( Be::Var* value ) override;

	/// Room ID, from the same ID space as game objects.
	AkUInt64 GetRoomID() const { return m_roomID; }
	/// Name shown in the Wwise profiler.
	const std::string& GetName() const { return m_name; }
	/// Reverb aux bus name. Empty means no reverb.
	const std::string& GetReverbAuxBus() const { return m_reverbAuxBus; }
	float GetReverbLevel() const { return m_reverbLevel; }
	float GetTransmissionLoss() const { return m_transmissionLoss; }
	float GetPriority() const { return m_priority; }
	float GetAuxSendLevelToSelf() const { return m_auxSendLevelToSelf; }
	bool GetKeepRegistered() const { return m_keepRegistered; }
	/// Room tone event. Empty means no room tone.
	const std::wstring& GetRoomToneEvent() const { return m_roomToneEvent; }
	/// True when the room and its volume object are both enabled.
	bool IsEnabled() const { return m_enabled && m_shapeEnabled; }
	/// Box of the room, set through ITr2BoxVolume.
	const AudUnitBox& GetBox() const { return m_box; }
	/// Whether the room is in Wwise.
	bool IsRegistered() const { return m_sentToWwise; }

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
	/// Enabled attribute of the room.
	bool m_enabled;
	/// Set through ITr2BoxVolume::SetEnabled.
	bool m_shapeEnabled;

	AudUnitBox m_box;
	bool m_warnedDegenerate;
	bool m_sentToWwise;
	bool m_registeredWithManager;

	/// Last parameters sent to Wwise.
	AkRoomParams m_sentRoomParams;
	std::string m_sentName;

	/// Room tone state, owned by AudRoomManager.
	/// Room tone event last posted. Empty when none.
	std::wstring m_postedRoomTone;
	AkPlayingID m_roomTonePlayingID;
	/// Room tone waiting for its SoundBanks.
	bool m_roomTonePending;
};

TYPEDEF_BLUECLASS( AudRoom );
