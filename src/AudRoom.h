////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Sep 2026
// Copyright (c) 2026 Fenris Creations
//

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

	/**
	 * @brief Stores the room's box and sends the room to Wwise.
	 *
	 * @param unitBoxToWorld Maps the unit box to world space.
	 */
	void SetTransform( const Matrix& unitBoxToWorld ) override;

	/**
	 * @brief Called when the volume object is enabled or disabled.
	 *
	 * @param enabled Whether the volume object is enabled.
	 */
	void SetEnabled( bool enabled ) override;

	/**
	 * @brief Clears the box and removes the room from Wwise.
	 */
	void Remove() override;

	/**
	 * @brief Called by Blue when an attribute changes.
	 *
	 * @param value The attribute that changed.
	 *
	 * @return Always true.
	 */
	bool OnModified( Be::Var* value ) override;

	/**
	 * @brief Room ID, from the same ID space as game objects.
	 */
	AkUInt64 GetRoomID() const { return m_roomID; }

	/**
	 * @brief Name shown in the Wwise profiler.
	 */
	const std::string& GetName() const { return m_name; }

	/**
	 * @brief Reverb aux bus name. Empty means no reverb.
	 */
	const std::string& GetReverbAuxBus() const { return m_reverbAuxBus; }

	/**
	 * @brief Send level [0, 1] from emitters in the room to the reverb bus.
	 */
	float GetReverbLevel() const { return m_reverbLevel; }

	/**
	 * @brief Transmission loss [0, 1] through the walls of the room.
	 */
	float GetTransmissionLoss() const { return m_transmissionLoss; }

	/**
	 * @brief The room with the highest priority wins when rooms overlap.
	 */
	float GetPriority() const { return m_priority; }

	/**
	 * @brief Send level [0, 1] from the room tone to the reverb bus.
	 */
	float GetAuxSendLevelToSelf() const { return m_auxSendLevelToSelf; }

	/**
	 * @brief Whether the room game object stays registered in Wwise.
	 */
	bool GetKeepRegistered() const { return m_keepRegistered; }

	/**
	 * @brief Room tone event. Empty means no room tone.
	 */
	const std::wstring& GetRoomToneEvent() const { return m_roomToneEvent; }

	/**
	 * @brief True when the room and its volume object are both enabled.
	 */
	bool IsEnabled() const { return m_enabled && m_shapeEnabled; }

	/**
	 * @brief Box of the room, set through ITr2BoxVolume.
	 */
	const AudUnitBox& GetBox() const { return m_box; }

	/**
	 * @brief Whether the room is in Wwise.
	 */
	bool IsRegistered() const { return m_sentToWwise; }

private:
	friend class AudRoomManager;

	/**
	 * @brief Registers with the room manager once it exists.
	 *
	 * @return True when the room is registered.
	 */
	bool EnsureRegistered();

	/**
	 * @brief Sends the room to Wwise, or removes it when disabled.
	 */
	void Sync();

	/**
	 * @brief Stores the box. Called by AudRoomManager under its lock.
	 *
	 * @param unitBoxToWorld Maps the unit box to world space.
	 */
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
