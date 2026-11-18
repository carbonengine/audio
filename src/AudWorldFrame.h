////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <CcpMutex.h>

#include <cstdint>

struct IEveAcousticGeometrySource;

/**
 * @brief The frame positions are sent to Wwise in, so static geometry stays static while the ego moves.
 *
 * Trinity's scene is centred on the ego, so everything static moves in it whenever the ego does. While active,
 * Wwise gets every position as scene + offset, where offset = renderReference - origin: absolute positions relative
 * to an origin that only moves on a rebase. Inactive, the offset is zero and positions go to Wwise as they come.
 *
 * Update() runs on the tick thread. GetOffset() can be called from any thread.
 */
class AudWorldFrame
{
public:
	struct Offset
	{
		double value[3] = {};
		/// Changes every time the offset does, so a position sent with an older one can be sent again.
		uint64_t generation = 0;
	};

	AudWorldFrame();

	/**
	 * @param source    Where the render reference comes from, or null.
	 * @param wanted    Whether the frame should be active.
	 * @param pinned    Whether static geometry is in Wwise, which a soft rebase would have to send again.
	 */
	void Update( IEveAcousticGeometrySource* source, bool wanted, bool pinned );
	void Reset();

	bool IsActive() const { return m_active; }
	Offset GetOffset() const;
	/// Changes whenever the origin does, so everything static sent to Wwise has to go again.
	uint64_t GetEpoch() const { return m_epoch; }
	/// The generation destiny reported with the reference, which tells when its keys go stale.
	uint64_t GetSourceGeneration() const { return m_sourceGeneration; }
	const double* GetOrigin() const { return m_origin; }
	uint64_t GetRebases() const { return m_rebases; }

private:
	void SetOffsetLocked( double x, double y, double z );

	double m_origin[3];
	uint64_t m_epoch;
	uint64_t m_sourceGeneration;
	uint64_t m_rebases;
	bool m_active;

	mutable CcpMutex m_mutex;
	Offset m_offset;
};
