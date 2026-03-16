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
 * @brief Registers meshes from Trinity as Spatial Audio geometry sets and manages their lifecycle.
 *
 * Implements the ITr2AudGeometry interface to register meshes to
 * AK::SpatialAudio as geeometry sets. A geometry set is a set of vertices, triangles,
 * and acoustic surfaces (see AkGeometryParams). Each geometry instance represents a unique
 * placement of a geometry set in the world with a transform — position,
 * orientation, and scale.
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
	 * @brief Registers a geometry set instance and places it in the world.
	 *
	 * @param geometrySetId  Shared geometry set identifier.
	 * @param instanceId     Unique geometry instance identifier.
	 * @param geometryData   Triangle mesh data.
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
