#include "StdAfx.h"
#include "Semaphore.h"

Semaphore::Semaphore()
{
	m_semaphore = ::CreateSemaphore( 0, 0, 1, 0 );
}

Semaphore::Semaphore( LONG initialCount, LONG maximumCount )
{
    m_semaphore = ::CreateSemaphore( 0, initialCount, maximumCount, 0 );
}


Semaphore::~Semaphore()
{
	::CloseHandle( m_semaphore );
}

bool Semaphore::Wait( DWORD timeout )
{
	return ::WaitForSingleObject( m_semaphore, timeout ) == 0;
}

void Semaphore::Signal()
{
	::ReleaseSemaphore( m_semaphore, 1, 0 );
}


