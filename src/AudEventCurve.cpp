#include "StdAfx.h"
#include "AudEventCurve.h"
#include "AudEmitter.h"
#include "CcpMath/include/Vector3.h"

BLUE_DEFINE_INTERFACE( ITriObserverLocal );

AudEventCurve::AudEventCurve( IRoot* lockobj ) :
	PARENTLOCK( m_keys ),
	m_time( 0 ),
	m_currentKeyIt( m_keys.begin() ),
	m_extrapolation( TRIEXT_NONE ),
	m_length( 0.0 ),
	m_localTime( 0.0 ),
	m_playOnLoad( false )
{
}

void AudEventCurve::UpdateValue( double time )
{
	if( m_length == 0.0f )
	{
		return;
	}

	double before = m_time;
	m_time = time;

	if( m_time < before )
	{
		// Time has moved backwards - can't rely on our caching
		m_currentKeyIt = m_keys.begin();

	}

	switch( m_extrapolation )
	{
		case TRIEXT_CYCLE:
			{
				float localNow = (float)fmod( m_time, (double)m_length );
				if( localNow < m_localTime )
				{
					// We've wrapped around
					m_currentKeyIt = m_keys.begin();
				}
				m_localTime = localNow;
			}
			break;

		default:
			m_localTime = (float)m_time;
			break;
	}

	while( (m_currentKeyIt != m_keys.end()) && (m_localTime >= (*m_currentKeyIt)->m_time) )
	{
		AudEventKey* currentKey = *m_currentKeyIt;

		if ( !currentKey->m_value.empty() )
		{
			m_audioEmitter->HandleEvent( currentKey->m_value.c_str() );
		}

		++m_currentKeyIt;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   This funtion is called prior to starting a curve playback. The EventCurve
//   needs to act upon it by reseting internal pointers
// --------------------------------------------------------------------------------
void AudEventCurve::Reset()
{
	m_currentKeyIt = m_keys.begin();
}

static bool CompareKeys( IRoot* context, AudEventKey* a, AudEventKey* b )
{
	return a->m_time < b->m_time;
}

void AudEventCurve::AddKey( float time, const std::wstring& evtName )
{
	AudEventKeyPtr key;
	if( !key.CreateInstance() )
	{
		return;
	}

	key->m_time = time;
	key->m_value = evtName.c_str();
	InsertKey(key);
}

void AudEventCurve::RemoveKey( int ix )
{
	m_keys.Remove( ix );
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
	m_currentKeyIt = m_keys.begin();
	if( m_keys.GetSize() > 0 )
	{
		m_length = m_keys.back()->m_time;
	}
}

int AudEventCurve::GetKeyCount()
{
	return (int)m_keys.GetSize();
}

bool AudEventCurve::Initialize()
{
	// Sort the keys here, just in case somebody tweaked the red file externally :-)
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
	m_currentKeyIt = m_keys.begin();
	if( m_keys.GetSize() > 0 )
	{
		m_length = m_keys.back()->m_time;
	}

	if (m_sourceTriObserver != NULL)
	{
		CreateAudioEmitter();
	}
	return true;
}

void AudEventCurve::CreateAudioEmitter()
{
	if (m_sourceTriObserver == NULL)
	{
		return;
	}
	IBluePlacementObserverPtr existingEmitter = m_sourceTriObserver->GetObserver();
	if (existingEmitter == NULL)
	{
		//Here no audioEmitter exists on the TriObserver so we need to create one.
		AudEmitterPtr audioEmitter;
		audioEmitter.CreateInstance( );
		audioEmitter->Py__init__( m_name );

		m_sourceTriObserver->SetObserver(audioEmitter);
		m_audioEmitter = audioEmitter;
	}
	else
	{
		//If there is already an existing audio emitter we use that.
		IBlueEventListenerPtr emitterPointer(BlueCastPtr( existingEmitter ) );
		m_audioEmitter = emitterPointer;
	}
	return;
	
}

float AudEventCurve::GetKeyTime( int ix )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		return m_keys[ix]->m_time;
	}
	return 0.0;
}

std::wstring AudEventCurve::GetKeyValue( int ix )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		return m_keys[ix]->m_value;
	}
	return std::wstring();
}

void AudEventCurve::SetKeyTime( int ix, float time )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		m_keys[ix]->m_time = time;
		m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );
		m_length = m_keys.back()->m_time;
	}
}

void AudEventCurve::SetKeyValue( int ix, std::wstring value )
{
	if( (ix < m_keys.GetSize()) && (ix >= 0) )
	{
		m_keys[ix]->m_value = value;
	}
}

void AudEventCurve::InsertKey( AudEventKey* key )
{
	m_keys.Insert( -1, key );
	m_keys.Sort( (IList::CompareFn)CompareKeys, NULL );

	m_currentKeyIt = m_keys.begin();
	m_length = m_keys.back()->m_time;
}

ITriObserverLocal* AudEventCurve::GetSourceTriObserver()
{
	return m_sourceTriObserver;
}

void AudEventCurve::SetSourceTriObserver(ITriObserverLocal* sourceTriObserver)
{
	m_sourceTriObserver = sourceTriObserver;
	CreateAudioEmitter();
}