#pragma once
#ifndef AudEventCurve_h
#define AudEventCurve_h


#include "AudEventKey.h"
#include "blue/Include/IBluePlacementObserver.h"
#include "trinity/include/ITriFunction.h"
#include "trinity/include/ITriCurveLength.h"
#include "blue/include/IBlueEventListener.h"
#include "CcpMath/include/Vector3.h"
#include "trinity/TriObserverLocal.h"

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
	void UpdateValue( double time );
	void Reset();

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITriEventCurve
	float Length() { return m_length; }

	void Sort();

	void AddKey( float time, const std::wstring& evtName );
	void InsertKey( AudEventKey* key );
	void RemoveKey( int ix );
	
	float GetKeyTime( int ix );
	std::wstring GetKeyValue( int ix );
	
	void SetKeyTime( int ix, float time );
	void SetKeyValue( int ix, std::wstring value );

	int GetKeyCount();

	ITriObserverLocal* GetSourceTriObserver();
	void SetSourceTriObserver( ITriObserverLocal* sourceTriObserver );

private:
	std::string m_name;
	double m_time;
	float m_localTime;
	std::wstring m_value;
	float m_length;

	bool m_playOnLoad;

	TRIEXTRAPOLATION m_extrapolation;
	PAudEventKeyVector m_keys;
	AudEventKeyVector::const_iterator m_currentKeyIt;

	IBlueEventListenerPtr m_audioEmitter;
	ITriObserverLocalPtr m_sourceTriObserver;

	void CreateAudioEmitter();
};

TYPEDEF_BLUECLASS( AudEventCurve );
#endif //AudEventCurve_h
