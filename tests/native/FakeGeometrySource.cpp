// Copyright © 2026 CCP ehf.

// Test-only stand-in for trinity, so the tests can register meshes the way EveAudioOccluder does.

#include <BlueExposure.h>
#include <Blue.h>
#include <ITr2AudGeometry.h>

BLUE_DEFINE_INTERFACE( ITr2AudGeometry );
BLUE_DECLARE_INTERFACE( ITr2AudGeometry );

// Registers boxes with carbon-audio's AudGeometry.
BLUE_CLASS( FakeGeometrySource ) : public IRoot
{
public:
	FakeGeometrySource( IRoot* lockobj = nullptr )
	{
		BeClasses->CreateInstanceFromName( "AudGeometry", BlueInterfaceIID<ITr2AudGeometry>(), reinterpret_cast<void**>( &m_geometry.p ) );
	}

	EXPOSE_TO_BLUE();

	bool IsConnected()
	{
		return m_geometry.p != nullptr;
	}

	// Every face has its own four vertices, like a render mesh with split normals.
	void AddBox( int geometrySetId, int instanceId, const Vector3& center, const Vector3& size )
	{
		Tr2AudGeometryData data;
		for( int axis = 0; axis < 3; ++axis )
		{
			for( float side : { -0.5f, 0.5f } )
			{
				const uint32_t first = static_cast<uint32_t>( data.m_vertices.size() );
				const float corners[4][2] = { { -0.5f, -0.5f }, { 0.5f, -0.5f }, { 0.5f, 0.5f }, { -0.5f, 0.5f } };
				for( const auto& corner : corners )
				{
					float position[3];
					position[axis] = side;
					position[( axis + 1 ) % 3] = corner[0];
					position[( axis + 2 ) % 3] = corner[1];
					data.m_vertices.push_back( MakeVector( position[0], position[1], position[2] ) );
				}
				for( uint32_t index : { 0u, 1u, 2u, 0u, 2u, 3u } )
				{
					data.m_indices.push_back( first + index );
				}
			}
		}
		Register( geometrySetId, instanceId, data, center, size );
	}

	void AddTriangle( int geometrySetId, int instanceId, const Vector3& center, const Vector3& size )
	{
		Tr2AudGeometryData data;
		data.m_vertices = { MakeVector( 0.0f, 0.0f, 0.0f ), MakeVector( 1.0f, 0.0f, 0.0f ), MakeVector( 0.0f, 1.0f, 0.0f ) };
		data.m_indices = { 0, 1, 2 };
		Register( geometrySetId, instanceId, data, center, size );
	}

	// One triangle with two corners at the same position on different vertices, which welding collapses.
	void AddCollapsedTriangle( int geometrySetId, int instanceId )
	{
		Tr2AudGeometryData data;
		data.m_vertices = { MakeVector( 0.0f, 0.0f, 0.0f ), MakeVector( -0.0f, 0.0f, 0.0f ), MakeVector( 1.0f, 0.0f, 0.0f ) };
		data.m_indices = { 0, 1, 2 };
		Register( geometrySetId, instanceId, data, MakeVector( 0.0f, 0.0f, 0.0f ), MakeVector( 1.0f, 1.0f, 1.0f ) );
	}

	// One good triangle next to one whose index points past the vertices.
	void AddTriangleWithBadIndex( int geometrySetId, int instanceId )
	{
		Tr2AudGeometryData data;
		data.m_vertices = { MakeVector( 0.0f, 0.0f, 0.0f ), MakeVector( 1.0f, 0.0f, 0.0f ), MakeVector( 0.0f, 1.0f, 0.0f ) };
		data.m_indices = { 0, 1, 2, 0, 1, 7 };
		Register( geometrySetId, instanceId, data, MakeVector( 0.0f, 0.0f, 0.0f ), MakeVector( 1.0f, 1.0f, 1.0f ) );
	}

	void Move( int geometrySetId, int instanceId, const Vector3& center, const Vector3& size )
	{
		if( m_geometry )
		{
			m_geometry->SetGeometryTransform( geometrySetId, instanceId, MakeTransform( center, size ) );
		}
	}

	void Remove( int geometrySetId, int instanceId )
	{
		if( m_geometry )
		{
			m_geometry->RemoveGeometry( geometrySetId, instanceId );
		}
	}

private:
	static Vector3 MakeVector( float x, float y, float z )
	{
		Vector3 v;
		v.x = x;
		v.y = y;
		v.z = z;
		return v;
	}

	static Matrix MakeTransform( const Vector3& center, const Vector3& size )
	{
		Matrix transform = {};
		transform._11 = size.x;
		transform._22 = size.y;
		transform._33 = size.z;
		transform._41 = center.x;
		transform._42 = center.y;
		transform._43 = center.z;
		transform._44 = 1.0f;
		return transform;
	}

	void Register( int geometrySetId, int instanceId, const Tr2AudGeometryData& data, const Vector3& center, const Vector3& size )
	{
		if( m_geometry )
		{
			m_geometry->SetGeometry( geometrySetId, instanceId, data, MakeTransform( center, size ) );
		}
	}

	ITr2AudGeometryPtr m_geometry;
};

TYPEDEF_BLUECLASS( FakeGeometrySource );
BLUE_DEFINE( FakeGeometrySource );

const Be::ClassInfo* FakeGeometrySource::ExposeToBlue()
{
	EXPOSURE_BEGIN( FakeGeometrySource, "Stands in for trinity in the tests, registering meshes with AudGeometry." )
		MAP_INTERFACE( FakeGeometrySource )
		MAP_METHOD_AND_WRAP
		(
			"IsConnected",
			IsConnected,
			"Whether carbon-audio's AudGeometry could be created."
		)
		MAP_METHOD_AND_WRAP
		(
			"AddBox",
			AddBox,
			"Registers a box as an instance of a geometry set.\n"
			":param geometrySetId: Geometry set the box belongs to.\n"
			":param instanceId: Instance to create.\n"
			":param center: Centre of the box.\n"
			":param size: Width, height and depth of the box."
		)
		MAP_METHOD_AND_WRAP
		(
			"AddTriangle",
			AddTriangle,
			"Registers a single triangle as an instance of a geometry set.\n"
			":param geometrySetId: Geometry set the triangle belongs to.\n"
			":param instanceId: Instance to create.\n"
			":param center: Position of the triangle.\n"
			":param size: Scale of the triangle."
		)
		MAP_METHOD_AND_WRAP
		(
			"AddCollapsedTriangle",
			AddCollapsedTriangle,
			"Registers a triangle that welding collapses, so the set has no usable triangles.\n"
			":param geometrySetId: Geometry set to create.\n"
			":param instanceId: Instance to create."
		)
		MAP_METHOD_AND_WRAP
		(
			"AddTriangleWithBadIndex",
			AddTriangleWithBadIndex,
			"Registers one good triangle and one with an index past the vertices.\n"
			":param geometrySetId: Geometry set to create.\n"
			":param instanceId: Instance to create."
		)
		MAP_METHOD_AND_WRAP
		(
			"Move",
			Move,
			"Moves an instance.\n"
			":param geometrySetId: Geometry set of the instance.\n"
			":param instanceId: Instance to move.\n"
			":param center: New centre of the box.\n"
			":param size: New width, height and depth of the box."
		)
		MAP_METHOD_AND_WRAP
		(
			"Remove",
			Remove,
			"Removes an instance.\n"
			":param geometrySetId: Geometry set of the instance.\n"
			":param instanceId: Instance to remove."
		)
	EXPOSURE_END()
}
