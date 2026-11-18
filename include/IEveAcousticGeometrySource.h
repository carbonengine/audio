////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Nov 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include <cstdint>

/**
 * @brief A static body with convex collision, in destiny's absolute frame (trinity's axes, right-handed).
 */
struct EveAcousticBody
{
	uint64_t bodyId;
	/// Nonzero. Bodies with the same key have the same components until the generation changes.
	uint64_t shapeKey;
	double position[3];
	/// Body to world, x y z w.
	double rotation[4];
	/// Around position, enclosing every hull, scale applied.
	double boundingRadius;
};

/**
 * @brief One collision component of a shape, in the body's frame.
 */
struct EveAcousticComponent
{
	/// Nonzero. Components with the same key have the same mesh until the generation changes.
	uint64_t meshKey;
	/// Body frame, scale applied.
	double translation[3];
	/// Component to body, x y z w.
	double rotation[4];
	/// Component frame, every hull, scale baked in.
	double boundsMin[3];
	double boundsMax[3];
	uint32_t hullCount;
	uint32_t vertexCount;
	uint32_t triangleCount;
	uint32_t reserved;
};

/**
 * @brief Where one convex hull lies in a mesh's vertex and triangle arrays.
 */
struct EveAcousticHull
{
	uint32_t firstVertex;
	uint32_t vertexCount;
	uint32_t firstTriangle;
	uint32_t triangleCount;
};

static_assert( sizeof( EveAcousticBody ) == 80 && sizeof( EveAcousticComponent ) == 128 && sizeof( EveAcousticHull ) == 16,
	"The layout is shared with destiny and must never change under this interface name." );

/**
 * @brief Interface destiny's ballpark implements so CarbonAudio can send its convex collision to Wwise as acoustic geometry.
 *
 * CarbonAudio pulls from it on its tick, on the Blue main thread. Callers own every array, and neither side keeps
 * anything past a call. Destiny has a copy of this header. Blue matches interfaces by name only, so the methods and
 * structs must never change under this name: a new layout needs a new name.
 */
BLUE_INTERFACE( IEveAcousticGeometrySource ) : public IRoot
{
	/**
	 * @brief The ego position trinity used for the last rendered frame, so audio space plus it is the absolute position.
	 *
	 * @param outPosition   Absolute position.
	 * @param outFrameTime  Time of that frame.
	 * @param outGeneration Always written. It changes whenever shape and mesh keys may be stale.
	 *
	 * @return False when there is none, for example before the first frame or on a server.
	 */
	virtual bool GetAudioSpaceOrigin( double outPosition[3], int64_t* outFrameTime, uint64_t* outGeneration ) = 0;

	/**
	 * @brief Static bodies with convex collision whose bounding sphere comes within radius of center. Free balls and the ego
	 * are left out.
	 *
	 * @param radius 0 or less for every body.
	 *
	 * @return The number of bodies. min(total, capacity) of them are written.
	 */
	virtual uint32_t GetAcousticBodies( const double center[3], double radius, EveAcousticBody* outBodies, uint32_t capacity ) = 0;

	/**
	 * @return The number of components of a shape. min(total, capacity) of them are written. 0 for an unknown key.
	 */
	virtual uint32_t GetAcousticComponents( uint64_t shapeKey, EveAcousticComponent* outComponents, uint32_t capacity ) = 0;

	/**
	 * @brief The hulls of a mesh in its component's frame, scale baked in.
	 *
	 * @param outVertices  x y z per vertex.
	 * @param outTriangles a b c per triangle, indexing the mesh's whole vertex array.
	 *
	 * @return False, and nothing written, for an unknown key or a capacity below the counts its component reports.
	 */
	virtual bool GetAcousticMesh( uint64_t meshKey, EveAcousticHull* outHulls, uint32_t hullCapacity,
		double* outVertices, uint32_t vertexCapacity, uint32_t* outTriangles, uint32_t triangleCapacity ) = 0;
};
