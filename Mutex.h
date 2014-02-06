#ifndef _MUTEX_H_
#define _MUTEX_H_

class Mutex
{
public:
	Mutex();
	~Mutex();

	bool Acquire( DWORD timeout = INFINITE );
	void Release();

private:
	HANDLE m_mutex;
};

#endif
