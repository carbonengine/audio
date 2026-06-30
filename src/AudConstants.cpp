// Copyright © 2014 CCP ehf.

#include "stdafx.h"
#include "AudConstants.h"

const Be::VarChooser TriExtrapolation[] =
{
	{ 
		"TRIEXT_NONE",
		BeCast(TRIEXT_NONE),
		"no comment" 
	},
	{ 
		"TRIEXT_CONSTANT",
		BeCast(TRIEXT_CONSTANT),
		"no comment" 
	},
	{ 
		"TRIEXT_GRADIENT",
		BeCast(TRIEXT_GRADIENT),
		"no comment" 
	},
	{ 
		"TRIEXT_CYCLE",
		BeCast(TRIEXT_CYCLE),
		"no comment" 
	},
	{0}
};