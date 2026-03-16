////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Jan 2026
// Copyright (c) 2026 CCP Games
//

#pragma once


#include <ITr2AudGeometry.h>
#include <ITr2AudEmitter.h>

#include <unordered_map>


/**
 * @brief Manages Wwise Spatial Audio geometry sets and geometry instances.
 *
 * Implements the ITr2AudGeometry interface to submit geometry to
 * AK::SpatialAudio. A geometry set is a logical set of vertices, triangles,
 * and acoustic surfaces (see AkGeometryParams). A geometry instance is a unique
 * placement of a geometry set in the world with a specified transform — position,
 * orientation, and scale (see AkGeometryInstanceParams).
 *
 * When @c AudOcclusionMode::On, geometry is registered with diffraction and
 * transmission enabled. When @c AudOcclusionMode::Off, geometry registration
 * is skipped entirely.
 */
BLUE_CLASS(AudGeometry) :
	public ITr2AudGeometry

{
public:
	AudGeometry(IRoot* lockobj = NULL);
	virtual ~AudGeometry();

	EXPOSE_TO_BLUE();


	// ITr2AudGeometry interface

	/**
	 * @brief Registers a ref-counted geometry set and places a geometry
	 *        instance in the world via AK::SpatialAudio::SetGeometryInstance.
	 *
	 * @param geometrySetId  Shared geometry set identifier.
	 * @param instanceId     Unique geometry instance identifier.
	 * @param geometryData   Triangle mesh.
	 * @param worldTransform World position, orientation and scale.
	 *
	 */
	void SetGeometry(
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Tr2AudGeometryData& geometryData,
		const Matrix& worldTransform) override;

	/**
	 * @brief Updates the world transform of an existing geometry instance
	 *
	 * @param geometrySetId  Geometry set the instance references.
	 * @param instanceId     Geometry instance to update.
	 * @param worldTransform New world-space position, orientation and scale.
	 *
	 */
	void SetGeometryTransform(
		uint64_t geometrySetId,
		uint64_t instanceId,
		const Matrix& worldTransform) override;

	/**
	 * @brief Removes a geometry instance and releases the geometry set when
	 *        its reference count reaches zero.
	 *
	 * @param geometrySetId  Geometry set to dereference.
	 * @param instanceId     Geometry instance to remove.
	 *
	 */
	void RemoveGeometry(
		uint64_t geometrySetId,
		uint64_t instanceId) override;

private:
	/// Builds AkGeometryInstanceParams from a geometry set ID and world transform.
	static AkGeometryInstanceParams MakeInstanceParams(
		uint64_t geometrySetId, const Matrix& worldTransform );

	/// Tracks how many active instances reference each geometry set.
	inline static std::unordered_map<uint64_t, uint32_t> s_geometrySetRefCounts;
};

TYPEDEF_BLUECLASS( AudGeometry );
