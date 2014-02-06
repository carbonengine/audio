#pragma once
#ifndef _AUDCONSTANTS_H_
#define _AUDCONSTANTS_H_

extern const Be::VarChooser TriExtrapolation[];

enum TRIEXTRAPOLATION { 
	TRIEXT_NONE = 0,
	TRIEXT_CONSTANT = 1,
	TRIEXT_GRADIENT = 2,
	TRIEXT_CYCLE = 3,
};


#endif