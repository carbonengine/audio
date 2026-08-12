////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jul 2026
// Copyright (c) 2026 Fernis Creations
//

#include "AudObstructionOcclusion.h"
#include "AudManager.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{
	/**
	 * @brief A translated occluder ready for a single line-of-sight pass.
	 *
	 * Carries how far its near surface sits from the sightline start. That distance is
	 * what lets an emitter stop scanning once the spheres are farther away than it is.
	 */
	struct SortableOccluder
	{
		uint64_t occluderID = 0;
		Vector3 center;
		float radius = 0.0f;
		float nearSurfaceDistance = 0.0f;
	};
}

AudObstructionOcclusion::AudObstructionOcclusion(AudManager* audioManager)	:
	m_audioManager(audioManager),
	m_originX(0.0),
	m_originY(0.0),
	m_originZ(0.0),
	m_sightlineSourceX(0.0),
	m_sightlineSourceY(0.0),
	m_sightlineSourceZ(0.0),
	m_hasSightlineSource(false),
	m_fadeRate(DEFAULT_FADE_RATE),
	m_blockedOcclusion(DEFAULT_BLOCKED_OCCLUSION),
	m_occluderRadiusScale(DEFAULT_OCCLUDER_RADIUS_SCALE),
	m_losRecomputeInterval(DEFAULT_LOS_RECOMPUTE_INTERVAL),
	m_hasUpdated(false),
	m_hasComputedLos(false),
	m_enabled(true),
	m_mutex("AudObstructionOcclusion", "m_mutex")
{}

AudObstructionOcclusion::~AudObstructionOcclusion()
{}

void AudObstructionOcclusion::Update()
{

	if (m_audioManager == nullptr || m_audioManager->GetState() != AudioState::Enabled)
	{
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	float deltaSeconds = 0.0f;

	if (m_hasUpdated)
	{
		deltaSeconds = std::chrono::duration<float>(now - m_lastUpdateTime).count();
	}
	m_lastUpdateTime = now;
	m_hasUpdated = true;

	CcpAutoMutex lock( m_mutex );

	if (SphereOcclusionActive())
	{
		ComputeSphereOcclusion(now);
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end();)
	{
		const AkGameObjectID emitterID = it->first;
		EmitterState& state = it->second;

		bool culled = false;
		const bool exists = m_audioManager->WithCallbackGameObject(emitterID, [&culled](AudGameObjResource* emitter) {
			culled = emitter->IsCulled();
			});

		if (!exists)
		{
			it = m_emitters.erase(it);
			continue;
		}

		const bool obstructionChanged = state.obstruction.Advance(deltaSeconds, m_fadeRate);
		const bool occlusionChanged = state.occlusion.Advance(deltaSeconds, m_fadeRate);

		if (culled)
		{
			// The emitter is outside Wwise's view. Keep fading so time stays
			// consistent, and make sure the values are resent once the emitter wakes up.
			state.needsSend = true;
			++it;
			continue;
		}

		if (obstructionChanged || occlusionChanged || state.needsSend)
		{
			state.needsSend = !SendToWwise(emitterID, state);
		}


		++it;
	}
}

float AudObstructionOcclusion::GetEmitterOcclusion(AkGameObjectID emitterID) const
{
	CcpAutoMutex lock(m_mutex);

	auto it = m_emitters.find(emitterID);
	if (it == m_emitters.end())
	{
		return 0.0f;
	}

	return it->second.occlusion.currentValue;
}

bool AudObstructionOcclusion::SendToWwise(AkGameObjectID emitterID, const EmitterState& state) const
{
	const AKRESULT result = AK::SoundEngine::SetObjectObstructionAndOcclusion(
		emitterID,
		LISTENER_GAME_OBJ_ID,
		state.obstruction.currentValue,
		state.occlusion.currentValue);
	return result == AK_Success;
}

void AudObstructionOcclusion::RemoveEmitter(AkGameObjectID emitterID)
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.erase(emitterID);
}

void AudObstructionOcclusion::Reset()
{
	CcpAutoMutex lock(m_mutex);
	m_emitters.clear();
	m_occluders.clear();
	m_blockingOccluderIDs.clear();
	m_hasUpdated = false;
	m_hasComputedLos = false;
}

void AudObstructionOcclusion::ClearAll()
{
	CcpAutoMutex lock(m_mutex);
	ClearAllTargetsLocked();
}

void AudObstructionOcclusion::ClearAllTargetsLocked()
{
	for (auto& pair : m_emitters)
	{
		pair.second.obstruction.SetTarget(0.0f);
		pair.second.occlusion.SetTarget(0.0f);
	}
}

bool AudObstructionOcclusion::IsEnabled() const
{
	return m_enabled;
}

void AudObstructionOcclusion::SetEnabled(bool value)
{
	if (m_enabled == value)
	{
		return;
	}

	m_enabled = value;
	if (!m_enabled)
	{
		ClearAll();
	}
}

float AudObstructionOcclusion::GetFadeRate() const
{
	return m_fadeRate;
}

void AudObstructionOcclusion::SetFadeRate(float value)
{
	m_fadeRate = std::max(value, 0.0f);
}

void AudObstructionOcclusion::SetOccluderSphere(uint64_t occluderID, double centerX, double centerY,
                                                double centerZ, float radius)
{
	if (radius <= 0.0f)
	{
		RemoveOccluderSphere(occluderID);
		return;
	}

	CcpAutoMutex lock(m_mutex);
	m_occluders[occluderID] = OccluderSphere{ centerX, centerY, centerZ, radius };
	// Occluder data changed; do not let the throttle delay the reaction to it.
	m_hasComputedLos = false;
}

void AudObstructionOcclusion::SetOccluderOrigin(double x, double y, double z)
{
	CcpAutoMutex lock(m_mutex);
	m_originX = x;
	m_originY = y;
	m_originZ = z;
	// Deliberately does not clear m_hasComputedLos: the origin moves with the
	// player every tick, so reacting to it immediately would defeat the throttle.
	// It is read when line of sight is next computed, which is soon enough.
}

void AudObstructionOcclusion::SetSightlineSource(double x, double y, double z)
{
	CcpAutoMutex lock(m_mutex);
	// Switching perspective should be audible immediately, but once the override is
	// in place it moves with the ship every tick, and reacting to that would defeat
	// the throttle just as reacting to the origin would (see SetOccluderOrigin).
	if (!m_hasSightlineSource)
	{
		m_hasComputedLos = false;
	}
	m_hasSightlineSource = true;
	m_sightlineSourceX = x;
	m_sightlineSourceY = y;
	m_sightlineSourceZ = z;
}

void AudObstructionOcclusion::ClearSightlineSource()
{
	CcpAutoMutex lock(m_mutex);
	if (!m_hasSightlineSource)
	{
		return;
	}
	m_hasSightlineSource = false;
	m_hasComputedLos = false;
}

bool AudObstructionOcclusion::HasSightlineSource() const
{
	CcpAutoMutex lock(m_mutex);
	return m_hasSightlineSource;
}

void AudObstructionOcclusion::RemoveOccluderSphere(uint64_t occluderID)
{
	CcpAutoMutex lock(m_mutex);
	if (m_occluders.erase(occluderID) == 0)
	{
		return;
	}
	m_hasComputedLos = false;
	m_blockingOccluderIDs.erase(occluderID);

	// The last sphere leaving ends sphere mode, so nothing would ever compute
	// the emitters back to clear. Fade them out here instead.
	if (m_occluders.empty())
	{
		ClearAllTargetsLocked();
	}
}

void AudObstructionOcclusion::ClearOccluderSpheres()
{
	CcpAutoMutex lock(m_mutex);
	if (m_occluders.empty())
	{
		return;
	}
	m_occluders.clear();
	m_blockingOccluderIDs.clear();
	m_hasComputedLos = false;
	ClearAllTargetsLocked();
}

float AudObstructionOcclusion::GetBlockedOcclusion() const
{
	return m_blockedOcclusion;
}

void AudObstructionOcclusion::SetBlockedOcclusion(float value)
{
	CcpAutoMutex lock(m_mutex);
	m_blockedOcclusion = std::clamp(value, 0.0f, 1.0f);
	m_hasComputedLos = false;
}

float AudObstructionOcclusion::GetOccluderRadiusScale() const
{
	return m_occluderRadiusScale;
}

void AudObstructionOcclusion::SetOccluderRadiusScale(float value)
{
	CcpAutoMutex lock(m_mutex);
	m_occluderRadiusScale = std::clamp(value, 0.0f, 1.0f);
	// Every sightline's answer can change, so do not make the throttle sit on it.
	// This is tuned by ear from a live client, where a delayed reaction reads as the
	// setting not having taken.
	m_hasComputedLos = false;
}

float AudObstructionOcclusion::GetLosRecomputeInterval() const
{
	return m_losRecomputeInterval;
}

void AudObstructionOcclusion::SetLosRecomputeInterval(float value)
{
	m_losRecomputeInterval = std::max(value, 0.0f);
}

int AudObstructionOcclusion::GetOccluderSphereCount() const
{
	CcpAutoMutex lock(m_mutex);
	return static_cast<int>(m_occluders.size());
}

std::vector<uint64_t> AudObstructionOcclusion::GetBlockingOccluderIDs() const
{
	CcpAutoMutex lock(m_mutex);
	if (!SphereOcclusionActive())
	{
		return {};
	}
	return std::vector<uint64_t>(m_blockingOccluderIDs.begin(), m_blockingOccluderIDs.end());
}

uint64_t AudObstructionOcclusion::GetBlockingOccluder(AkGameObjectID emitterID) const
{
	CcpAutoMutex lock(m_mutex);

	if (m_audioManager == nullptr)
	{
		return 0;
	}

	Vector3 sightlineStart;
	if (!GetSightlineStartLocked(sightlineStart))
	{
		return 0;
	}

	Vector3 emitterPosition;
	bool emitterIsPlaced = false;
	const bool haveEmitter = m_audioManager->WithCallbackGameObject(emitterID,
		[&emitterPosition, &emitterIsPlaced](AudGameObjResource* emitter) {
			emitterPosition = emitter->GetPosition();
			emitterIsPlaced = emitter->HasUsableWorldPosition();
		});

	if (!haveEmitter || !emitterIsPlaced)
	{
		return 0;
	}

	for (const auto& occluderEntry : m_occluders)
	{
		const TranslatedOccluder translated = TranslateToAudioSpace(occluderEntry.second);
		if (SegmentHitsSphere(sightlineStart, emitterPosition, translated.center, translated.radius))
		{
			return occluderEntry.first;
		}
	}

	return 0;
}

double AudObstructionOcclusion::GetOriginX() const
{
	CcpAutoMutex lock(m_mutex);
	return m_originX;
}

double AudObstructionOcclusion::GetOriginY() const
{
	CcpAutoMutex lock(m_mutex);
	return m_originY;
}

double AudObstructionOcclusion::GetOriginZ() const
{
	CcpAutoMutex lock(m_mutex);
	return m_originZ;
}

bool AudObstructionOcclusion::SphereOcclusionActive() const
{
	return m_enabled
		&& !m_occluders.empty()
		&& m_audioManager != nullptr
		&& !m_audioManager->GetSpatialAudioGeometryEnabled();
}

AudObstructionOcclusion::TranslatedOccluder AudObstructionOcclusion::TranslateToAudioSpace(const OccluderSphere& sphere) const
{
	return { Vector3(static_cast<float>(sphere.centerX - m_originX),
	                 static_cast<float>(sphere.centerY - m_originY),
	                 static_cast<float>(sphere.centerZ - m_originZ)),
	         sphere.radius * m_occluderRadiusScale };
}

bool AudObstructionOcclusion::GetSightlineStartLocked(Vector3& start) const
{
	if (m_hasSightlineSource)
	{
		start = Vector3(static_cast<float>(m_sightlineSourceX - m_originX),
		                static_cast<float>(m_sightlineSourceY - m_originY),
		                static_cast<float>(m_sightlineSourceZ - m_originZ));
		return true;
	}

	Vector3 listenerPosition;
	bool listenerIsPlaced = false;
	const bool haveListener = m_audioManager->WithCallbackGameObject(LISTENER_GAME_OBJ_ID,
		[&listenerPosition, &listenerIsPlaced](AudGameObjResource* listener) {
			listenerPosition = listener->GetPosition();
			listenerIsPlaced = listener->HasUsableWorldPosition();
		});
	// Until a camera drives the listener it sits at the initial spawn position
	if (!haveListener || !listenerIsPlaced)
	{
		return false;
	}

	start = listenerPosition;
	return true;
}

void AudObstructionOcclusion::ComputeSphereOcclusion(std::chrono::steady_clock::time_point now)
{
	if (m_hasComputedLos &&
		std::chrono::duration<float>(now - m_lastLosComputeTime).count() < m_losRecomputeInterval)
	{
		return;
	}
	m_lastLosComputeTime = now;
	m_hasComputedLos = true;
	m_blockingOccluderIDs.clear();

	Vector3 sightlineStart;
	if (!GetSightlineStartLocked(sightlineStart))
	{
		return;
	}

	// Snapshot the positioned emitters so targets are not set while the manager's game object map is locked.
	std::vector<std::pair<AkGameObjectID, Vector3>> emitters;
	m_audioManager->ForEachCallbackGameObject(
		[&emitters](AkGameObjectID gameObjID, AudGameObjResource* gameObj) {
			if (gameObjID == LISTENER_GAME_OBJ_ID || !gameObj->HasUsableWorldPosition())
			{
				return;
			}
			// A culled emitter is outside Wwise's view, so working out its sightline buys nothing. 
			if (gameObj->IsCulled())
			{
				return;
			}
			emitters.emplace_back(gameObjID, gameObj->GetPosition());
		});

	// Translate the occluders into audio space once, rather than once per emitter.
	std::vector<SortableOccluder> occluders;
	occluders.reserve(m_occluders.size());
	for (const auto& occluderEntry : m_occluders)
	{
		const TranslatedOccluder translated = TranslateToAudioSpace(occluderEntry.second);

		SortableOccluder sortable;
		sortable.occluderID = occluderEntry.first;
		sortable.center = translated.center;
		sortable.radius = translated.radius;

		const Vector3 fromStart = translated.center - sightlineStart;
		sortable.nearSurfaceDistance =
			std::sqrt(Dot(fromStart, fromStart)) - translated.radius;

		occluders.push_back(sortable);
	}

	// Nearest surface first, so each emitter can stop scanning early below. The game
	// registers occluders across its whole occluder range while emitters are usually
	// a few kilometres out, so ordering the list once per pass saves testing most of
	// it once per emitter.
	std::sort(occluders.begin(), occluders.end(),
		[](const SortableOccluder& lhs, const SortableOccluder& rhs) {
			return lhs.nearSurfaceDistance < rhs.nearSurfaceDistance;
		});

	for (const auto& [emitterID, position] : emitters)
	{
		const Vector3 toEmitter = position - sightlineStart;
		const float segmentLength = std::sqrt(Dot(toEmitter, toEmitter));

		bool blocked = false;
		for (const auto& occluder : occluders)
		{
			// A sightline reaches no farther than its end, so a sphere whose near
			// surface is beyond that cannot touch it. The list is sorted, so neither
			// can anything after it.
			if (occluder.nearSurfaceDistance > segmentLength)
			{
				break;
			}

			if (SegmentHitsSphere(sightlineStart, position, occluder.center, occluder.radius))
			{
				blocked = true;
				m_blockingOccluderIDs.insert(occluder.occluderID);
				break;
			}
		}

		auto it = m_emitters.find(emitterID);
		if (it == m_emitters.end())
		{
			// Do not track emitters that have never been blocked; most never are.
			if (!blocked)
			{
				continue;
			}
			EmitterState& state = m_emitters[emitterID];
			// The first value snaps: an emitter that spawns behind cover starts
			// muffled instead of fading in from clear.
			state.occlusion.SetTarget(m_blockedOcclusion);
			state.occlusion.SnapToTarget();
			continue;
		}

		it->second.obstruction.SetTarget(0.0f);
		it->second.occlusion.SetTarget(blocked ? m_blockedOcclusion : 0.0f);
	}
}

bool AudObstructionOcclusion::SegmentHitsSphere(const Vector3& segmentStart, const Vector3& segmentEnd,
                                                const Vector3& sphereCenter, float sphereRadius)
{
	const Vector3 segment = segmentEnd - segmentStart;
	const Vector3 toCenter = sphereCenter - segmentStart;
	const float radiusSq = sphereRadius * sphereRadius;

	// A sphere the segment's start is inside is not between that point and anything.
	// Occluder radii are coarse bounding spheres and the player routinely flies inside
	// a large one, so counting that as blockage would muffle the entire world at once.
	if (Dot(toCenter, toCenter) <= radiusSq)
	{
		return false;
	}

	const float segmentLengthSq = Dot(segment, segment);
	// Where the centre falls along the segment, scaled by the segment's length so no
	// division is needed to compare it against the ends.
	const float centerProjection = Dot(toCenter, segment);

	// An emitter inside a sphere needs more care than the start did, because the two
	// situations that produce it want opposite answers. A turret or beam emitter mounted
	// on an object sits inside that object's own bounding sphere while being in plain
	// sight, and muffling it would be permanent. A sound on the far side of the object,
	// like a mining beam terminating against a wall, is genuinely behind it.
	//
	// They are told apart by how much of the sphere the sound has already crossed. The
	// sightline is deepest inside the sphere where the centre projects onto it, so an
	// emitter short of that point has barely entered and one past it has crossed more
	// than half. Note this asks where the emitter is along the sightline, not which side
	// of the centre it is on: an emitter well off to one side is still only just inside.
	//
	// Falling through then reports blocked, since an endpoint inside a sphere always
	// intersects it.
	const Vector3 toCenterFromEnd = sphereCenter - segmentEnd;
	if (Dot(toCenterFromEnd, toCenterFromEnd) <= radiusSq
		&& centerProjection >= segmentLengthSq)
	{
		return false;
	}

	// Degenerate segment: blocked only if the shared point is inside the sphere.
	float t = 0.0f;
	if (segmentLengthSq > 0.0f)
	{
		t = std::clamp(centerProjection / segmentLengthSq, 0.0f, 1.0f);
	}

	const Vector3 closestToCenter = sphereCenter - (segmentStart + segment * t);
	return Dot(closestToCenter, closestToCenter) < radiusSq;
}

void AudObstructionOcclusion::FadingValue::SetTarget(float target)
{
	targetValue = std::clamp(target, 0.0f, 1.0f);
}

bool AudObstructionOcclusion::FadingValue::Advance(float deltaSeconds, float fadeRate)
{
	if (currentValue == targetValue)
	{
		return false;
	}

	if (fadeRate <= 0.0f)
	{
		currentValue = targetValue;
		return true;
	}

	const float oldValue = currentValue;
	const float step = fadeRate * deltaSeconds;

	if (oldValue > targetValue)
	{
		currentValue = std::clamp(oldValue - step, targetValue, oldValue);
	}
	else
	{
		currentValue = std::clamp(oldValue + step, oldValue, targetValue);
	}
	return currentValue != oldValue;
}
