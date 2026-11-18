// Copyright © 2026 CCP ehf.

// Test-only stand-in for destiny's ballpark as a source of convex collision, set as AudManager.acousticGeometrySource.

#include <BlueExposure.h>
#include <Blue.h>
#include <IEveAcousticGeometrySource.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

BLUE_DEFINE_INTERFACE( IEveAcousticGeometrySource );

BLUE_CLASS( FakeAcousticGeometrySource ) : public IEveAcousticGeometrySource
{
public:
	FakeAcousticGeometrySource( IRoot* lockobj = nullptr ) {}

	EXPOSE_TO_BLUE();

	bool GetAudioSpaceOrigin( double outPosition[3], int64_t* outFrameTime, uint64_t* outGeneration ) override
	{
		*outGeneration = static_cast<uint64_t>( m_generation );
		if( !m_hasOrigin )
		{
			return false;
		}
		std::copy( m_origin, m_origin + 3, outPosition );
		*outFrameTime = 0;
		return true;
	}

	uint32_t GetAcousticBodies( const double center[3], double radius, EveAcousticBody* outBodies, uint32_t capacity ) override
	{
		++m_bodiesCalls;
		uint32_t total = 0;
		for( const auto& entry : m_bodies )
		{
			const EveAcousticBody& body = entry.second;
			if( radius > 0.0 )
			{
				const double dx = body.position[0] - center[0], dy = body.position[1] - center[1], dz = body.position[2] - center[2];
				if( std::sqrt( dx * dx + dy * dy + dz * dz ) - body.boundingRadius > radius )
				{
					continue;
				}
			}
			if( total < capacity )
			{
				outBodies[total] = body;
			}
			++total;
		}
		return total;
	}

	uint32_t GetAcousticComponents( uint64_t shapeKey, EveAcousticComponent* outComponents, uint32_t capacity ) override
	{
		++m_componentsCalls;
		const auto it = m_shapes.find( static_cast<int>( shapeKey ) );
		if( it == m_shapes.end() )
		{
			return 0;
		}
		const uint32_t total = static_cast<uint32_t>( it->second.size() );
		std::copy_n( it->second.begin(), total < capacity ? total : capacity, outComponents );
		return total;
	}

	bool GetAcousticMesh( uint64_t meshKey, EveAcousticHull* outHulls, uint32_t hullCapacity,
		double* outVertices, uint32_t vertexCapacity, uint32_t* outTriangles, uint32_t triangleCapacity ) override
	{
		++m_meshCalls;
		const auto it = m_meshes.find( static_cast<int>( meshKey ) );
		if( it == m_meshes.end() )
		{
			return false;
		}
		const Mesh& mesh = it->second;
		if( hullCapacity < mesh.hulls.size() || vertexCapacity < mesh.vertices.size() / 3 || triangleCapacity < mesh.triangles.size() / 3 )
		{
			return false;
		}
		std::copy( mesh.hulls.begin(), mesh.hulls.end(), outHulls );
		std::copy( mesh.vertices.begin(), mesh.vertices.end(), outVertices );
		std::copy( mesh.triangles.begin(), mesh.triangles.end(), outTriangles );
		return true;
	}

	void SetOrigin( double x, double y, double z )
	{
		m_origin[0] = x;
		m_origin[1] = y;
		m_origin[2] = z;
		m_hasOrigin = true;
	}

	void ClearOrigin()
	{
		m_hasOrigin = false;
	}

	// A body rotated angle radians about Y.
	void AddBody( int bodyId, int shapeKey, double x, double y, double z, double angle )
	{
		EveAcousticBody body = {};
		body.bodyId = static_cast<uint64_t>( bodyId );
		body.shapeKey = static_cast<uint64_t>( shapeKey );
		body.position[0] = x;
		body.position[1] = y;
		body.position[2] = z;
		body.rotation[1] = std::sin( angle * 0.5 );
		body.rotation[3] = std::cos( angle * 0.5 );
		body.boundingRadius = 100.0;
		m_bodies[bodyId] = body;
	}

	void RemoveBody( int bodyId )
	{
		m_bodies.erase( bodyId );
	}

	// A component translated by (x, y, z) in its body, rotated angle radians about Y, using a box mesh.
	void AddComponent( int shapeKey, int meshKey, double x, double y, double z, double angle )
	{
		const auto meshIt = m_meshes.find( meshKey );
		if( meshIt == m_meshes.end() )
		{
			return;
		}
		const Mesh& mesh = meshIt->second;

		EveAcousticComponent component = {};
		component.meshKey = static_cast<uint64_t>( meshKey );
		component.translation[0] = x;
		component.translation[1] = y;
		component.translation[2] = z;
		component.rotation[1] = std::sin( angle * 0.5 );
		component.rotation[3] = std::cos( angle * 0.5 );
		for( int axis = 0; axis < 3; ++axis )
		{
			component.boundsMin[axis] = -mesh.halfSize[axis];
			component.boundsMax[axis] = mesh.halfSize[axis];
		}
		component.hullCount = static_cast<uint32_t>( mesh.hulls.size() );
		component.vertexCount = static_cast<uint32_t>( mesh.vertices.size() / 3 );
		component.triangleCount = static_cast<uint32_t>( mesh.triangles.size() / 3 );
		m_shapes[shapeKey].push_back( component );
	}

	// A closed box centred on its component, as one convex hull, 8 vertices and 12 triangles.
	void DefineBoxMesh( int meshKey, double sizeX, double sizeY, double sizeZ )
	{
		Mesh mesh;
		mesh.halfSize[0] = sizeX * 0.5;
		mesh.halfSize[1] = sizeY * 0.5;
		mesh.halfSize[2] = sizeZ * 0.5;
		for( int i = 0; i < 8; ++i )
		{
			mesh.vertices.push_back( ( i & 1 ) ? mesh.halfSize[0] : -mesh.halfSize[0] );
			mesh.vertices.push_back( ( i & 2 ) ? mesh.halfSize[1] : -mesh.halfSize[1] );
			mesh.vertices.push_back( ( i & 4 ) ? mesh.halfSize[2] : -mesh.halfSize[2] );
		}
		mesh.triangles = { 0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4, 2, 6, 3, 3, 6, 7, 0, 4, 2, 2, 4, 6, 1, 3, 5, 3, 7, 5 };
		mesh.hulls.push_back( { 0, 8, 0, 12 } );
		m_meshes[meshKey] = mesh;
	}

	// A mesh whose triangle indexes a vertex past the end, so nothing of it is usable.
	void DefineBadMesh( int meshKey )
	{
		Mesh mesh;
		mesh.halfSize[0] = mesh.halfSize[1] = mesh.halfSize[2] = 1.0;
		mesh.vertices = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };
		mesh.triangles = { 0, 1, 9 };
		mesh.hulls.push_back( { 0, 3, 0, 1 } );
		m_meshes[meshKey] = mesh;
	}

private:
	struct Mesh
	{
		std::vector<EveAcousticHull> hulls;
		std::vector<double> vertices;
		std::vector<uint32_t> triangles;
		double halfSize[3] = {};
	};

	double m_origin[3] = {};
	bool m_hasOrigin = false;
	int m_generation = 1;
	int m_bodiesCalls = 0;
	int m_componentsCalls = 0;
	int m_meshCalls = 0;
	std::map<int, EveAcousticBody> m_bodies;
	std::map<int, std::vector<EveAcousticComponent>> m_shapes;
	std::map<int, Mesh> m_meshes;
};

TYPEDEF_BLUECLASS( FakeAcousticGeometrySource );
BLUE_DEFINE( FakeAcousticGeometrySource );

const Be::ClassInfo* FakeAcousticGeometrySource::ExposeToBlue()
{
	EXPOSURE_BEGIN( FakeAcousticGeometrySource, "Stands in for destiny's ballpark as a source of convex collision in the tests." )
		MAP_INTERFACE( IEveAcousticGeometrySource )
		MAP_INTERFACE( FakeAcousticGeometrySource )
		MAP_ATTRIBUTE( "generation", m_generation, "Changing it tells audio every key it holds is stale.", Be::READWRITE )
		MAP_ATTRIBUTE( "bodiesCalls", m_bodiesCalls, "How many times the bodies were asked for.", Be::READ )
		MAP_ATTRIBUTE( "componentsCalls", m_componentsCalls, "How many times a shape's components were asked for.", Be::READ )
		MAP_ATTRIBUTE( "meshCalls", m_meshCalls, "How many times a mesh was asked for.", Be::READ )
		MAP_METHOD_AND_WRAP( "SetOrigin", SetOrigin, "Sets the render reference, the ego's absolute position." )
		MAP_METHOD_AND_WRAP( "ClearOrigin", ClearOrigin, "Removes the render reference, like before the first frame." )
		MAP_METHOD_AND_WRAP( "AddBody", AddBody, "Adds or replaces a static body: bodyId, shapeKey, x, y, z, angle about Y in radians." )
		MAP_METHOD_AND_WRAP( "RemoveBody", RemoveBody, "Removes a body." )
		MAP_METHOD_AND_WRAP( "AddComponent", AddComponent, "Adds a component to a shape: shapeKey, meshKey, x, y, z, angle about Y in radians. Define the mesh first." )
		MAP_METHOD_AND_WRAP( "DefineBoxMesh", DefineBoxMesh, "Defines a closed box mesh of the given size, centred on its component." )
		MAP_METHOD_AND_WRAP( "DefineBadMesh", DefineBadMesh, "Defines a mesh with an out of range index." )
	EXPOSURE_END()
}
