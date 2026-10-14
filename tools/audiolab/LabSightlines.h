// Copyright © 2026 CCP ehf.

#pragma once

#include "LabScene.h"

#include <IEveObstructionQuery.h>

namespace AudioLab
{
	/// Registers the lab's own Blue classes; once, after BootBlue.
	void RegisterLabClasses();

	/// The lab's answer to CarbonAudio's sightline query, which destiny gives in the game: a sightline is blocked when it
	/// crosses one of the scene's walls. CarbonAudio holds the query weakly, so the caller keeps it. CarbonAudio asks
	/// from Tick(), on the thread that edits the scene.
	BluePtr<IEveObstructionQuery> CreateSightlineQuery( const LabScene& scene );
}
