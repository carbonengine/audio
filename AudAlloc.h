/* 
	*************************************************************************************

	AudAlloc.h

	Author:    Andri Mar
	Created:   December 2008
	OS:        Win32
	Project:   Audio2

	Description:   

		Memory allocation hooks for Wwise. Insert own memory handlers to change
		how Wwise handles memory allocation


	Dependencies:

		Blue

	(c) CCP 2008

	*************************************************************************************
*/

//-----------------------------------------------------------------------------
// Custom alloc/free functions. These are declared as "extern" in AkMemoryMgr.h
// and MUST be defined by the game developer.
//-----------------------------------------------------------------------------
#pragma once
#ifndef _AUDALLOC_H_
#define _AUDALLOC_H_

#include "Audio2.h"

namespace AK
{
	void * AllocHook( size_t in_size )
	{
		return CCP_MALLOC( "AK::AllocHook", in_size );
	}
	void FreeHook( void * in_ptr )
	{
		CCP_FREE( in_ptr );
	}
	void * VirtualAllocHook( void * in_pMemAddress, size_t in_size, DWORD in_dwAllocationType, DWORD in_dwProtect )
	{
		return VirtualAlloc( in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect );
	}
	void VirtualFreeHook( void * in_pMemAddress, size_t in_size, DWORD in_dwFreeType )
	{
		VirtualFree( in_pMemAddress, in_size, in_dwFreeType );
	}
}
#endif