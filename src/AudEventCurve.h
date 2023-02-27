#pragma once
#ifndef AudEventCurve_h
#define AudEventCurve_h

#include <IBluePlacementObserver.h>
#include <ITriConstants.h>
#include <ITriCurveLength.h>
#include <ITriFunction.h>
#include <ITriObserverLocal.h>
#include <Vector3.h>

#include "AudEmitter.h"
#include "AudEventKey.h"
#include "ITr2AudEmitter.h"

BLUE_DECLARE( AudEventCurve );

class AudEventCurve:
     public ITriFunction,
	 public IInitialize,
	 public ITriCurveLength
{
public:
    EXPOSE_TO_BLUE();
    AudEventCurve( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time ) override;
	void Reset() override;

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize() override;

	//////////////////////////////////////////////////////////////////////////
	// ITriEventCurve
	float Length() override { return m_length; }

	void Sort();

	void AddKey( float time, const std::wstring& evtName );
	void InsertKey( AudEventKey* key );
	void RemoveKey( int ix );
	
	float GetKeyTime( int ix );
	std::wstring GetKeyValue( int ix );
	
	void SetKeyTime( int ix, float time );
	void SetKeyValue( int ix, const std::wstring& value );

	int GetKeyCount();

	ITriObserverLocal* GetSourceTriObserver();
	void SetSourceTriObserver( ITriObserverLocal* sourceTriObserver );

private:
	std::string m_name;
	std::wstring m_queuedEvent;
	double m_time;
	float m_localTime;
	std::wstring m_value;
	float m_length;

	bool m_playOnLoad;

	TRIEXTRAPOLATION m_extrapolation;
	PAudEventKeyVector m_keys;
	AudEventKeyVector::const_iterator m_currentKeyIt;

	AudEmitterPtr m_audioEmitter;
	ITriObserverLocalPtr m_sourceTriObserver;

	void CreateAudioEmitter();
};

TYPEDEF_BLUECLASS( AudEventCurve );
#endif //AudEventCurve_h
