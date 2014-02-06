#include "StdAfx.h"

#include "AudConstants.h"

#define VAL(v) BeCast(v)

const Be::VarChooser TriExtrapolation[] =
{
	{ 
		"TRIEXT_NONE",     
		VAL(TRIEXT_NONE),     
		"no comment" 
	},
	{ 
		"TRIEXT_CONSTANT", 
		VAL(TRIEXT_CONSTANT), 
		"no comment" 
	},
	{ 
		"TRIEXT_GRADIENT", 
		VAL(TRIEXT_GRADIENT), 
		"no comment" 
	},
	{ 
		"TRIEXT_CYCLE",    
		VAL(TRIEXT_CYCLE),    
		"no comment" 
	},
	{0}
};