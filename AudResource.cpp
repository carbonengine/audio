#include "stdafx.h"
#include "AudResource.h"

AudResource::AudResource()
{
	RegisterResource();
}

AudResource::~AudResource()
{
	UnregisterResource();
}

static ResourceSet s_registeredResources;
void AudResource::RegisterResource()
{
	s_registeredResources.insert( this );
}

void AudResource::UnregisterResource()
{
	s_registeredResources.erase( this );	// This is ok because s_registeredResources is a set.
}

void AudResource::RecreateResources()
{
	ResourceSet::iterator resEnd = s_registeredResources.end();
	for( ResourceSet::iterator it = s_registeredResources.begin(); it != resEnd; ++it)
	{
		(*it)->CreateWwiseObject();
	}
}

void AudResource::LogInfoOnResources()
{
	ResourceSet::iterator resEnd = s_registeredResources.end();
	for( ResourceSet::iterator it = s_registeredResources.begin(); it != resEnd; ++it)
	{
		(*it)->LogInfo();
	}
}