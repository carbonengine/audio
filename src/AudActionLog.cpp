#include "stdafx.h"
#include "AudActionLog.h"

AudActionRecordPostEvent::AudActionRecordPostEvent( Be::Time time, AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name ) :
	m_time( time ),
	m_emitterID( emitterID ),
	m_playID( playID ),
	m_eventID( eventID ),
	m_name( name )
{
}

PyObject* AudActionRecordPostEvent::ToPyObject()
{
	return Py_BuildValue( "sKKKKu", "AudActionRecordPostEvent", m_time, m_emitterID, m_playID, m_eventID, m_name.c_str() );
}

AudActionRecordExecuteActionOnPlayingID::AudActionRecordExecuteActionOnPlayingID( Be::Time time, AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action ) :
	m_time( time ),
	m_emitterID( emitterID ),
	m_playID( playID ),
	m_action( action )
{
}

PyObject* AudActionRecordExecuteActionOnPlayingID::ToPyObject()
{
	return Py_BuildValue( "sKKKu", "AudActionRecordExecuteActionOnPlayingID", m_time, m_emitterID, m_playID, m_action.c_str() );
}

AudActionRecordSetSwitch::AudActionRecordSetSwitch( Be::Time time, AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state ) :
	m_time( time ),
	m_emitterID( emitterID ),
	m_group( group ),
	m_state( state )
{
}

PyObject* AudActionRecordSetSwitch::ToPyObject()
{
	return Py_BuildValue( "sKKuu", "AudActionRecordSetSwitch", m_time, m_emitterID, m_group.c_str(), m_state.c_str() );
}

AudActionRecordSetState::AudActionRecordSetState( Be::Time time, const std::wstring& group, const std::wstring& state ) :
	m_time( time ),
	m_group( group ),
	m_state( state )
{
}

PyObject* AudActionRecordSetState::ToPyObject()
{
	return Py_BuildValue( "sKuu", "AudActionRecordSetState", m_time, m_group.c_str(), m_state.c_str() );
}

AudActionRecordSetRTPC::AudActionRecordSetRTPC( Be::Time time, AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID ) :
	m_time( time ),
	m_emitterID( emitterID ),
	m_name( name ),
	m_value( value ),
	m_playID( playID )
{
}
	
PyObject* AudActionRecordSetRTPC::ToPyObject()
{
	return Py_BuildValue( "sKKufK", "AudActionRecordSetRTPC", m_time, m_emitterID, m_name.c_str(), m_value, m_playID );
}

AudActionLogCB::AudActionLogCB( IRoot* lockobj ) :
	m_newRecordCallback(),
	m_queue(),
	m_mutex( "AudActionLogCB", "m_mutex" )
{
}

AudActionLogCB::~AudActionLogCB()
{
	while( !m_queue.empty() )
	{
		delete m_queue.front();
		m_queue.pop();
	}
}

void AudActionLogCB::LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name )
{
	CcpAutoMutex mutex( m_mutex );
	m_queue.push( new AudActionRecordPostEvent( BeOS->GetActualTime(), emitterID, playID, eventID, name ) );
}

void AudActionLogCB::LogExecuteActionOnPlayingID( AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action )
{
	CcpAutoMutex mutex( m_mutex );
	m_queue.push( new AudActionRecordExecuteActionOnPlayingID( BeOS->GetActualTime(), emitterID, playID, action ) );
}

void AudActionLogCB::LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state )
{
	CcpAutoMutex mutex( m_mutex );
	m_queue.push( new AudActionRecordSetSwitch( BeOS->GetActualTime(), emitterID, group, state ) );
}

void AudActionLogCB::LogSetState( const std::wstring& group, const std::wstring& state )
{
	CcpAutoMutex mutex( m_mutex );
	m_queue.push( new AudActionRecordSetState( BeOS->GetActualTime(), group, state ) );
}

void AudActionLogCB::LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID )
{
	CcpAutoMutex mutex( m_mutex );
	m_queue.push( new AudActionRecordSetRTPC( BeOS->GetActualTime(), emitterID, name, value, playID ) );
}

void AudActionLogCB::Flush()
{
	CcpAutoMutex mutex( m_mutex );
	while( m_newRecordCallback && !m_queue.empty() )
	{
		auto record = m_queue.front()->ToPyObject();
		m_newRecordCallback.CallVoid( record );
		Py_DecRef( record );
		delete m_queue.front();
		m_queue.pop();
	}
}

void AudActionLogCB::RegisterCallback( BlueScriptCallback callback )
{
	m_newRecordCallback = callback;
}
