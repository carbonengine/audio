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
#include <sstream>
#include <vector>

namespace
{
	/// An occluder translated into audio space for a single line-of-sight pass,
	/// with how far its near surface sits from the listener. That distance is what
	/// lets an emitter stop scanning once the spheres are farther away than it is.
	struct TranslatedOccluder
	{
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

	if (InSphereMode())
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

bool AudObstructionOcclusion::SetObstructionOcclusion(AkGameObjectID emitterID, float obstruction, float occlusion)
{
	if (!m_enabled)
	{
		return false;
	}

	if (m_audioManager == nullptr || m_audioManager->GetState() != AudioState::Enabled)
	{
		return false;
	}

	// Values are relative to the listener, so setting them on the listener itself is meaningless.
	if (emitterID == LISTENER_GAME_OBJ_ID)
	{
		return false;
	}

	// We need to ask AudioManager about emitters that actually exist.
	if (!m_audioManager->WithCallbackGameObject(emitterID, [](AudGameObjResource*) {}))
	{
		return false;
	}

	CcpAutoMutex lock(m_mutex);

	// While occluder spheres drive the occlusion, per-emitter values would fight
	// with what Update() computes, so they are rejected instead of merged.
	if (InSphereMode())
	{
		return false;
	}

	const auto [entry, isNewEmitter] = m_emitters.try_emplace(emitterID);
	EmitterState& state = entry->second;

	state.obstruction.SetTarget(obstruction);
	state.occlusion.SetTarget(occlusion);

	if (isNewEmitter)
	{
		state.obstruction.SnapToTarget();
		state.occlusion.SnapToTarget();
	}

	return true;
}

bool AudObstructionOcclusion::SetEmitterLineOfSightBlockage(AkGameObjectID emitterID, float blockage)
{
	// When Acoustics is On its transmission already attenuates, so skip occlusion to avoid stacking.
	// Might change in the future with the addition of volumes.
	const bool acousticsEnabled = m_audioManager != nullptr && m_audioManager->GetSpatialAudioGeometryEnabled();

	float occlusion = 0.0f;
	if (!acousticsEnabled)
	{
		occlusion = blockage;
	}

	return SetObstructionOcclusion(emitterID, 0.0f, occlusion);
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

void AudObstructionOcclusion::RemoveOccluderSphere(uint64_t occluderID)
{
	CcpAutoMutex lock(m_mutex);
	if (m_occluders.erase(occluderID) == 0)
	{
		return;
	}
	m_hasComputedLos = false;

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

std::string AudObstructionOcclusion::DescribeEmitterOcclusion(AkGameObjectID emitterID) const
{
	CcpAutoMutex lock(m_mutex);

	std::ostringstream out;
	out.precision(1);
	out << std::fixed;

	if (m_audioManager == nullptr)
	{
		return "no audio manager";
	}

	Vector3 listenerPosition;
	bool haveListener = false;
	bool listenerIsPlaced = false;
	Vector3 emitterPosition;
	bool haveEmitter = false;
	bool emitterIsPlaced = false;

	m_audioManager->ForEachCallbackGameObject(
		[&](AkGameObjectID gameObjID, AudGameObjResource* gameObj) {
			if (gameObjID == LISTENER_GAME_OBJ_ID)
			{
				haveListener = true;
				listenerPosition = gameObj->GetPosition();
				listenerIsPlaced = gameObj->HasReceivedPosition()
					&& IsUsableWorldPosition(listenerPosition);
			}
			else if (gameObjID == emitterID)
			{
				haveEmitter = true;
				emitterPosition = gameObj->GetPosition();
				emitterIsPlaced = gameObj->HasReceivedPosition()
					&& IsUsableWorldPosition(emitterPosition);
			}
		});

	if (!haveEmitter)
	{
		return "emitter does not exist";
	}
	if (!haveListener)
	{
		return "listener is not registered";
	}

	out << "occluders=" << m_occluders.size()
		<< " sphereMode=" << (InSphereMode() ? "yes" : "no")
		<< " radiusScale=" << m_occluderRadiusScale
		<< " origin=(" << m_originX << ", " << m_originY << ", " << m_originZ << ")"
		<< " listener=(" << listenerPosition.x << ", " << listenerPosition.y << ", "
		<< listenerPosition.z << ")" << (listenerIsPlaced ? "" : " [UNPLACED]")
		<< " emitter=(" << emitterPosition.x << ", " << emitterPosition.y << ", "
		<< emitterPosition.z << ")" << (emitterIsPlaced ? "" : " [UNPLACED]");

	const auto tracked = m_emitters.find(emitterID);
	out << " occlusion=";
	if (tracked == m_emitters.end())
	{
		out << "untracked";
	}
	else
	{
		out << tracked->second.occlusion.currentValue
			<< "->" << tracked->second.occlusion.targetValue;
	}

	if (!listenerIsPlaced || !emitterIsPlaced)
	{
		return out.str();
	}

	// How far away the emitter is. Worth printing: an emitter on a star or a planet is
	// millions of kilometres out, and a sightline that long crosses the whole local
	// occluder field, so it reads as blocked for reasons that have nothing to do with
	// what you can see.
	const Vector3 toEmitter = emitterPosition - listenerPosition;
	out << " range=" << std::sqrt(Dot(toEmitter, toEmitter));

	// The rest mirrors ComputeSphereOcclusion, shrunken radii included. A diagnostic
	// reporting raw radii would explain a different answer than the emitter actually got.
	for (const auto& occluderEntry : m_occluders)
	{
		const OccluderSphere& sphere = occluderEntry.second;
		const Vector3 center(static_cast<float>(sphere.centerX - m_originX),
		                     static_cast<float>(sphere.centerY - m_originY),
		                     static_cast<float>(sphere.centerZ - m_originZ));
		const float scaledRadius = sphere.radius * m_occluderRadiusScale;
		if (SegmentHitsSphere(listenerPosition, emitterPosition, center, scaledRadius))
		{
			out << " blockedBy=" << occluderEntry.first
				<< " at=(" << center.x << ", " << center.y << ", " << center.z << ")"
				<< " radius=" << sphere.radius << " scaledRadius=" << scaledRadius;
			return out.str();
		}
	}

	out << " blockedBy=none";
	return out.str();
}

bool AudObstructionOcclusion::InSphereMode() const
{
	return m_enabled
		&& !m_occluders.empty()
		&& m_audioManager != nullptr
		&& !m_audioManager->GetSpatialAudioGeometryEnabled();
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

	Vector3 listenerPosition;
	bool listenerIsPlaced = false;
	const bool haveListener = m_audioManager->WithCallbackGameObject(LISTENER_GAME_OBJ_ID,
		[&listenerPosition, &listenerIsPlaced](AudGameObjResource* listener) {
			listenerPosition = listener->GetPosition();
			listenerIsPlaced = IsUsableWorldPosition(listenerPosition);
		});
	// Until a camera drives the listener it sits on the spawn sentinel, and every
	// segment test against it would be meaningless rather than merely wrong.
	if (!haveListener || !listenerIsPlaced)
	{
		return;
	}

	// Snapshot the positioned emitters so targets are not set while the
	// manager's game object map is locked.
	std::vector<std::pair<AkGameObjectID, Vector3>> emitters;
	m_audioManager->ForEachCallbackGameObject(
		[&emitters](AkGameObjectID gameObjID, AudGameObjResource* gameObj) {
			if (gameObjID == LISTENER_GAME_OBJ_ID || !gameObj->HasReceivedPosition())
			{
				return;
			}
			// A culled emitter is outside Wwise's view, so working out its sightline
			// buys nothing. Update() already resends a culled emitter's values when it
			// wakes, and the pass after that retargets it, so it is stale for at most
			// one recompute interval.
			if (gameObj->IsCulled())
			{
				return;
			}
			const Vector3 position = gameObj->GetPosition();
			if (!IsUsableWorldPosition(position))
			{
				return;
			}
			emitters.emplace_back(gameObjID, position);
		});

	// Translate the occluders into audio space once, rather than once per emitter.
	// The subtraction is what the double precision is for; the result is local to
	// the player and small enough for a float to carry.
	std::vector<TranslatedOccluder> occluders;
	occluders.reserve(m_occluders.size());
	for (const auto& occluderEntry : m_occluders)
	{
		const OccluderSphere& sphere = occluderEntry.second;
		TranslatedOccluder translated;
		translated.center = Vector3(static_cast<float>(sphere.centerX - m_originX),
		                            static_cast<float>(sphere.centerY - m_originY),
		                            static_cast<float>(sphere.centerZ - m_originZ));
		// Shrunk once here, so every test below and the culling that skips them agree
		// on how big this sphere is.
		translated.radius = sphere.radius * m_occluderRadiusScale;

		const Vector3 fromListener = translated.center - listenerPosition;
		translated.nearSurfaceDistance =
			std::sqrt(Dot(fromListener, fromListener)) - translated.radius;

		occluders.push_back(translated);
	}

	// Nearest surface first, so each emitter can stop scanning early below. The game
	// registers occluders across its whole occluder range while emitters are usually
	// a few kilometres out, so ordering the list once per pass saves testing most of
	// it once per emitter.
	std::sort(occluders.begin(), occluders.end(),
		[](const TranslatedOccluder& lhs, const TranslatedOccluder& rhs) {
			return lhs.nearSurfaceDistance < rhs.nearSurfaceDistance;
		});

	for (const auto& [emitterID, position] : emitters)
	{
		const Vector3 toEmitter = position - listenerPosition;
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

			if (SegmentHitsSphere(listenerPosition, position, occluder.center, occluder.radius))
			{
				blocked = true;
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
			// First value snaps, matching SetObstructionOcclusion: an emitter that
			// spawns behind cover starts muffled instead of fading in from clear.
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

	// A sphere the listener is inside is not between the listener and anything. Occluder
	// radii are coarse bounding spheres and the player routinely flies inside a large
	// one, so counting that as blockage would muffle the entire world at once.
	if (Dot(toCenter, toCenter) <= radiusSq)
	{
		return false;
	}

	const float segmentLengthSq = Dot(segment, segment);
	// Where the centre falls along the segment, scaled by the segment's length so no
	// division is needed to compare it against the ends.
	const float centerProjection = Dot(toCenter, segment);

	// An emitter inside a sphere needs more care than the listener did, because the two
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
