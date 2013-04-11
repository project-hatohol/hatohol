#include "Synchronizer.h"

Synchronizer::Synchronizer(void)
{
	g_mutex_init(&g_mutex);
}

Synchronizer::~Synchronizer()
{
	g_mutex_clear(&g_mutex);
}

void Synchronizer::lock(void)
{
	g_mutex_lock(&g_mutex);
}

void Synchronizer::unlock(void)
{
	g_mutex_unlock(&g_mutex);
}

void Synchronizer::forceUnlock(void)
{
	// We wanted to write the following code, but that is just 'unlock()'
	// if (!trylock())
	//	unlock();
	// else
	//	unlock();

	unlock();
}

gboolean Synchronizer::trylock(void)
{
	return g_mutex_trylock(&g_mutex);
}

void Synchronizer::wait(void)
{
	lock();
	unlock();
}

