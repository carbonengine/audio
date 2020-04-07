////////////////////////////////////////////////////////////
//
//    Creator:   Eric Nielsen
//    Created:   April 2020
//    Copyright: CCP 2020
//
//    Description:
//      An interface intended for use by audio2 to expose relevant functions
//      related to audio emitters.

#pragma once

#ifndef ITr2AudEmitter_h_
#define ITr2AudEmitter_h_

BLUE_INTERFACE( ITr2AudEmitter ) : public IRoot
{
	virtual int SetPosition( const Vector3& front, const Vector3& top, const Vector3& pos ) = 0;
	virtual void SetName( const std::string name ) = 0;
	virtual unsigned int SendEvent( const std::wstring& name ) = 0;
};

#endif
