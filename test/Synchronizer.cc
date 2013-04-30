#include "Synchronizer.h"
using namespace mlpl;

Synchronizer::Synchronizer(void)
: m_mutex(NULL)
{
	m_mutex = new MutexLock();
}

Synchronizer::~Synchronizer()
{
	if (m_mutex)
		delete m_mutex;
}

void Synchronizer::lock(void)
{
	m_mutex->lock();
}

void Synchronizer::unlock(void)
{
	m_mutex->unlock();
}

void Synchronizer::reset(void)
{
	if (m_mutex)
		delete m_mutex;
	m_mutex = new MutexLock();
}

bool Synchronizer::trylock(void)
{
	return m_mutex->trylock();
}

bool Synchronizer::isLocked(void)
{
	bool locked = false;
	if (trylock())
		unlock();
	else
		locked = true;
	return locked;
}

void Synchronizer::wait(void)
{
	lock();
	unlock();
}

