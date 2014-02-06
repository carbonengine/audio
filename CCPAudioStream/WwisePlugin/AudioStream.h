//////////////////////////////////////////////////////////////////////
//
// AudioStream.h
//
// Audio Stream Wwise plugin: Main header file for the Audio Stream DLL.
//
// Copyright (c) 2011 CCP / Andrew Beck / All Rights Reserved, modified from AudioKinetic Sample Plug-in Audio Input
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

// CAudioStreamApp
// See AudioStream.cpp for the implementation of this class
//

class CAudioStreamApp : public CWinApp
{
	float **m_fTestBuffers;

public:
	CAudioStreamApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int  ExitInstance(); 

	DECLARE_MESSAGE_MAP()
};
