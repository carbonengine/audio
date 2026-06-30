// Copyright © 2014 CCP ehf.

#include "windows.h"

#define EVEFILEDESC "CCP Audio Engine\0"
#ifndef _DEBUG
#define EVEINTFILENAME "_audio2\0"
#define EVEFILENAME "_audio2.dll\0"
#else
#define EVEINTFILENAME "_audio2_d\0"
#define EVEFILENAME "_audio2_d.dll\0"
#endif
#define EVEFILETYPE VFT_DLL

#include "autoversion.h"
//standard file version thing
#include "../version/evebuildver.h"
