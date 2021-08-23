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

#ifndef _WIN32
#include <sys/mman.h>
#endif // _WIN32

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

	void * VirtualAllocHook( void * in_pMemAddress, size_t in_size, AkUInt32 in_dwAllocationType, AkUInt32 in_dwProtect )
	{
#ifdef _WIN32
		return VirtualAlloc( in_pMemAddress, in_size, in_dwAllocationType, in_dwProtect );
#else
        // ref : https://stackoverflow.com/questions/30057381/c-porting-virtualfree-in-os-x
#pragma message "Theoretical VirtualAlloc implementation for posix platforms. Disabled by current m_deviceSettings.ePoolAttributes setting."
        void* ptr = mmap(in_pMemAddress, in_size, (PROT_READ | PROT_WRITE), (MAP_FIXED | MAP_SHARED | MAP_ANON), -1, 0);
        msync(in_pMemAddress, in_size, (MS_SYNC | MS_INVALIDATE));
        return ptr;
#endif // _WIN32
	}
	void VirtualFreeHook( void * in_pMemAddress, size_t in_size, AkUInt32 in_dwFreeType )
	{
#ifdef _WIN32
		VirtualFree( in_pMemAddress, in_size, in_dwFreeType );
#else
#pragma message "Theoretical VirtualFree implementation for posix platforms. Disabled by current m_deviceSettings.ePoolAttributes setting."
        mmap(in_pMemAddress, in_size, PROT_NONE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
        msync(in_pMemAddress, in_size, MS_SYNC | MS_INVALIDATE);
#endif // _WIN32
	}
}
#endif