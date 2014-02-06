/* 
	*************************************************************************************

	AudResource.h

	Author:    Andri Mar
	Created:   February 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Generic audio2 resource, handles registration and unregistration.


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDRESOURCE_H_
#define _AUDRESOURCE_H_

#include "Audio2.h"

#include <set>

class AudResource
{
public:
	AudResource();
	virtual ~AudResource();

	void RegisterResource();
	void UnregisterResource();

	static void RecreateResources();
	static void LogInfoOnResources();
protected:
	virtual void CreateWwiseObject() = 0;
	virtual void LogInfo() = 0;
};

typedef std::set<AudResource*> ResourceSet;

#endif