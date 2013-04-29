#include "Synchronizer.h"

Synchronizer::Synchronizer(void)
{
	g_static_mutex_init(&g_mutex);
}

Synchronizer::~Synchronizer()
{
	g_static_mutex_free(&g_mutex);
}

void Synchronizer::lock(void)
{
	g_static_mutex_lock(&g_mutex);
}

void Synchronizer::unlock(void)
{
	g_static_mutex_unlock(&g_mutex);
}

void Synchronizer::reset(void)
{
	g_static_mutex_init(&g_mutex);
}

bool Synchronizer::trylock(void)
{
	return g_static_mutex_trylock(&g_mutex);
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

