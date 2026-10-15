////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 CCP Games
//

#pragma once

#include <ITr2BoxVolume.h>

#include "AudUnitBox.h"

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SpatialAudio/Common/AkSpatialAudioTypes.h>

#include <string>

/**
 * @brief A Wwise Spatial Audio portal, placed as a unit box through ITr2BoxVolume, with the front room on the box's local -Z side.
 */
BLUE_CLASS( AudPortal ) :
	public ITr2BoxVolume,
	public INotify
{
public:
	AudPortal( IRoot* lockobj = NULL );
	virtual ~AudPortal();

	EXPOSE_TO_BLUE();

	// ITr2BoxVolume
	void SetTransform( const Matrix& unitBoxToWorld ) override;
	void SetEnabled( bool enabled ) override;
	void Remove() override;

	// INotify
	bool OnModified( Be::Var* value ) override;

	/// Portal ID, from the same ID space as game objects.
	AkUInt64 GetPortalID() const { return m_portalID; }
	/// Name shown in the Wwise profiler.
	const std::string& GetName() const { return m_name; }
	/// Whether sound passes through the portal, e.g. an open door.
	bool IsOpen() const { return m_open; }
	float GetAdjacentRoomBleed() const { return m_adjacentRoomBleed; }
	/// True when the portal and its volume object are both enabled.
	bool IsEnabled() const { return m_enabled && m_shapeEnabled; }
	/// Box of the portal, set through ITr2BoxVolume.
	const AudUnitBox& GetBox() const { return m_box; }
	/// Whether the portal is in Wwise.
	bool IsRegistered() const { return m_sentToWwise; }
	/// Room on the front side, as sent to Wwise.
	AkUInt64 GetFrontRoomID() const { return m_frontRoomID; }
	/// Room on the back side, as sent to Wwise.
	AkUInt64 GetBackRoomID() const { return m_backRoomID; }

private:
	friend class AudRoomManager;

	/// Registers with the room manager once it exists.
	bool EnsureRegistered();
	/// Stores the box. Called by AudRoomManager under its lock.
	void ApplyTransform( const Matrix& unitBoxToWorld );

	AkUInt64 m_portalID;

	std::string m_name;
	bool m_open;
	float m_adjacentRoomBleed;
	/// Enabled attribute of the portal.
	bool m_enabled;
	/// Set through ITr2BoxVolume::SetEnabled.
	bool m_shapeEnabled;

	AudUnitBox m_box;
	bool m_warnedDegenerate;
	bool m_sentToWwise;
	bool m_registeredWithManager;

	/// Last parameters sent to Wwise.
	AkPortalParams m_sentPortalParams;
	std::string m_sentName;
	AkUInt64 m_frontRoomID;
	AkUInt64 m_backRoomID;
	/// Rooms found at the faces on the last update.
	AkUInt64 m_foundFrontRoomID;
	AkUInt64 m_foundBackRoomID;
	/// Updates in a row the same rooms were found.
	int m_foundTicks;
	/// Set after a failed Wwise call is logged, so it is not logged again every update.
	bool m_failureLogged;
};

TYPEDEF_BLUECLASS( AudPortal );
