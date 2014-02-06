#include "StdAfx.h"
#include "Mutex.h"

Mutex::Mutex()
{
	m_mutex = ::CreateMutex( 0, FALSE, 0 );
}

Mutex::~Mutex()
{
	::CloseHandle( m_mutex );
}

bool Mutex::Acquire( DWORD timeout )
{
	return WaitForSingleObject( m_mutex, timeout ) == 0;
}

void Mutex::Release()
{
	ReleaseMutex( m_mutex );
}

