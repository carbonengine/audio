// Copyright © 2021 CCP ehf.

#pragma once
#ifndef _AUDACTIONLOG_H_
#define _AUDACTIONLOG_H_

#include "Audio2.h"

#include <string>
#include <queue>

struct AudActionRecord
{
	virtual ~AudActionRecord(){};
	virtual PyObject* ToPyObject() = 0;
};

struct AudActionRecordPostEvent : public AudActionRecord
{
	Be::Time m_time;
	AkGameObjectID m_emitterID;
	AkPlayingID m_playID;
	AkUniqueID m_eventID;
	std::wstring m_name;

	AudActionRecordPostEvent( Be::Time time = 0, AkGameObjectID emitterID = AK_INVALID_UNIQUE_ID, AkPlayingID playID = AK_INVALID_UNIQUE_ID, AkUniqueID eventID = AK_INVALID_UNIQUE_ID, const std::wstring& name = std::wstring() );
	PyObject* ToPyObject() override;
};

struct AudActionRecordExecuteActionOnPlayingID: public AudActionRecord
{
	Be::Time m_time;
	AkGameObjectID m_emitterID;
	AkPlayingID m_playID;
	std::wstring m_action;

	AudActionRecordExecuteActionOnPlayingID( Be::Time time = 0, AkGameObjectID emitterID = AK_INVALID_UNIQUE_ID, AkPlayingID playID = AK_INVALID_UNIQUE_ID, const std::wstring& action = std::wstring() );
	PyObject* ToPyObject() override;
};

struct AudActionRecordSetSwitch : public AudActionRecord
{
	Be::Time m_time;
	AkGameObjectID m_emitterID;
	std::wstring m_group;
	std::wstring m_state;

	AudActionRecordSetSwitch( Be::Time time = AK_INVALID_UNIQUE_ID, AkGameObjectID emitterID = AK_INVALID_UNIQUE_ID, const std::wstring& group = std::wstring(), const std::wstring& state = std::wstring() );
	PyObject* ToPyObject() override;
};

struct AudActionRecordSetState : public AudActionRecord
{
	Be::Time m_time;
	std::wstring m_group;
	std::wstring m_state;

	AudActionRecordSetState( Be::Time time = AK_INVALID_UNIQUE_ID, const std::wstring& = std::wstring(), const std::wstring& state = std::wstring() );
	PyObject* ToPyObject() override;
};

struct AudActionRecordSetRTPC : public AudActionRecord
{
	Be::Time m_time;
	AkGameObjectID m_emitterID;
	std::wstring m_name;
	float m_value;
	AkPlayingID m_playID;

	AudActionRecordSetRTPC( Be::Time time = AK_INVALID_UNIQUE_ID, AkGameObjectID emitterID = AK_INVALID_UNIQUE_ID, const std::wstring& name = std::wstring(), float value = 0.0f, AkPlayingID playID = AK_INVALID_PLAYING_ID );
	PyObject* ToPyObject() override;
};

BLUE_INTERFACE( IAudActionLog ) :
	public IRoot
{
	virtual void LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name ) = 0;
	virtual void LogExecuteActionOnPlayingID( AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action ) = 0;
	virtual void LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state ) = 0;
	virtual void LogSetState( const std::wstring& group, const std::wstring& state ) = 0;
	virtual void LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID = AK_INVALID_PLAYING_ID ) = 0;

	virtual void Flush() = 0;
};

BLUE_CLASS( AudActionLogCB ) :
	public IAudActionLog
{
public:
	AudActionLogCB( IRoot* lockobj = NULL );
	~AudActionLogCB();

	EXPOSE_TO_BLUE();

	// Add a new record.
	void LogPostEvent( AkGameObjectID emitterID, AkPlayingID playID, AkUniqueID eventID, const std::wstring& name ) override;
	void LogExecuteActionOnPlayingID( AkGameObjectID emitterID, AkPlayingID playID, const std::wstring& action ) override;
	void LogSetSwitch( AkGameObjectID emitterID, const std::wstring& group, const std::wstring& state ) override;
	void LogSetState( const std::wstring& group, const std::wstring& state ) override;
	void LogSetRTPC( AkGameObjectID emitterID, const std::wstring& name, float value, AkPlayingID playID = AK_INVALID_PLAYING_ID ) override;

	void Flush() override;

	// Register callback
	void RegisterCallback( BlueScriptCallback callback );

private:
	BlueScriptCallback m_newRecordCallback;
	std::queue<AudActionRecord*> m_queue;
	CcpMutex m_mutex;
};

TYPEDEF_BLUECLASS( AudActionLogCB );

#endif // _AUDACTIONLOG_H_
