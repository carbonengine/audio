#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

// Simple wrapper for a Win32 semaphore
class Semaphore
{
public:
	Semaphore();
    Semaphore( LONG initialCount, LONG maximumCount );
	~Semaphore();

	bool Wait( DWORD timeout = INFINITE );
	void Signal();

private:
	HANDLE m_semaphore;
};

#endif
