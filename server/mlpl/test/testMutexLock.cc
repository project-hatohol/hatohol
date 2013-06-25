#include <string>
#include <vector>
using namespace std;

#include <glib.h>
#include <cutter.h>
#include <cppcutter.h>

#include "MutexLock.h"
using namespace mlpl;

namespace testMutexLock {

static const int COUNT_MAX = 100;

struct CounterThreadArg {
	volatile int count;
	MutexLock lock;
};

static void *counterThread(void *_arg)
{
	CounterThreadArg *arg = static_cast<CounterThreadArg *>(_arg);
	while (arg->count < COUNT_MAX)
		arg->count++;
	arg->lock.unlock();
	return NULL;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_make(void)
{
	MutexLock lock;
}

void test_lockUnlock(void)
{
	MutexLock lock;
	lock.lock();
	lock.unlock();
}

void test_lockWait(void)
{
	pthread_t thread;
	CounterThreadArg arg;
	arg.count = 0;
	arg.lock.lock();
	cppcut_assert_equal(0,
	                    pthread_create(&thread, NULL, counterThread, &arg));
	cppcut_assert_equal(0, pthread_detach(thread));

	// wait for unlock() by the created thread.
	arg.lock.lock();

	cppcut_assert_equal(COUNT_MAX, (const int)arg.count);
}

void test_trylock(void)
{
	MutexLock lock;
	lock.lock();
	cppcut_assert_equal(false, lock.trylock()); 
	lock.unlock();
	cppcut_assert_equal(true, lock.trylock()); 
	cppcut_assert_equal(false, lock.trylock()); 
}


} // namespace testMutexLock
