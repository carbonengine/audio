// Copyright © 2026 CCP ehf.

#include "LabSelection.h"

#include <algorithm>

namespace AudioLab
{
	bool Selection::Contains( int id ) const
	{
		return std::find( m_ids.begin(), m_ids.end(), id ) != m_ids.end();
	}

	int Selection::Count( LabKind kind ) const
	{
		return static_cast<int>( std::count_if( m_ids.begin(), m_ids.end(), [this, kind]( int id ) { return m_scene.KindOf( id ) == kind; } ) );
	}

	std::string Selection::Describe() const
	{
		if( m_ids.size() == 1 )
		{
			return m_scene.NameOf( m_ids.front() );
		}

		const auto count = []( int n, const char* one, const char* many ) { return std::to_string( n ) + " " + ( n == 1 ? one : many ); };
		std::vector<std::string> parts;
		if( const int emitters = Count( LabKind::Emitter ) )
		{
			parts.push_back( count( emitters, "emitter", "emitters" ) );
		}
		if( const int rooms = Count( LabKind::Room ) )
		{
			parts.push_back( count( rooms, "room", "rooms" ) );
		}
		if( const int portals = Count( LabKind::Portal ) )
		{
			parts.push_back( count( portals, "portal", "portals" ) );
		}
		if( const int walls = Count( LabKind::Wall ) )
		{
			parts.push_back( count( walls, "wall", "walls" ) );
		}
		if( Contains( LabScene::LISTENER_ID ) )
		{
			parts.push_back( "the listener" );
		}
		std::string text;
		for( size_t i = 0; i < parts.size(); ++i )
		{
			text += ( i == 0 ? "" : i + 1 == parts.size() ? " and " : ", " ) + parts[i];
		}
		return text;
	}

	void Selection::Select( int id )
	{
		m_ids.clear();
		if( id != LabScene::NO_ID )
		{
			m_ids.push_back( id );
			RequestShow();
		}
	}

	void Selection::Toggle( int id )
	{
		if( id == LabScene::NO_ID )
		{
			return;
		}
		const auto selected = std::find( m_ids.begin(), m_ids.end(), id );
		if( selected != m_ids.end() )
		{
			m_ids.erase( selected );
			return;
		}
		m_ids.push_back( id );
		RequestShow();
	}

	void Selection::Add( int id )
	{
		if( id != LabScene::NO_ID && !Contains( id ) )
		{
			m_ids.push_back( id );
		}
	}

	void Selection::Prune()
	{
		m_ids.erase( std::remove_if( m_ids.begin(), m_ids.end(), [this]( int id ) { return m_scene.KindOf( id ) == LabKind::None; } ), m_ids.end() );
	}

	void Selection::RemoveFromScene()
	{
		for( const int id : m_ids )
		{
			m_scene.Remove( id );
		}
		m_ids.clear();
	}

	void Selection::Play( bool play )
	{
		for( const int id : m_ids )
		{
			if( LabEmitter* emitter = m_scene.FindEmitter( id ) )
			{
				play ? m_scene.Play( *emitter ) : m_scene.Stop( *emitter );
			}
		}
	}

	void Selection::RequestShow()
	{
		m_showRequested = true;
		m_scrollRequested = true;
	}

	bool Selection::TakeShowRequest()
	{
		const bool requested = m_showRequested;
		m_showRequested = false;
		return requested;
	}
}
