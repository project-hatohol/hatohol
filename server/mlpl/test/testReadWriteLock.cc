#include <string>
#include <vector>
using namespace std;

#include <glib.h>
#include <cutter.h>
#include <cppcutter.h>
#include <sched.h>

#include "ReadWriteLock.h"
using namespace mlpl;

namespace testReadWriteLock {

struct ReadThreadArg {
	volatile int numNotReady;
	volatile long val;
	ReadWriteLock lock;
};

static void *readThread(void *_arg)
{
	ReadThreadArg *arg = static_cast<ReadThreadArg *>(_arg);
	__sync_sub_and_fetch(&arg->numNotReady, 1);
	arg->lock.readLock();
	void *ret = reinterpret_cast<void *>(arg->val);
	arg->lock.unlock();
	return ret;
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_make(void)
{
	ReadWriteLock lock;
}

void test_readLockUnlock(void)
{
	ReadWriteLock lock;
	lock.readLock();
	lock.unlock();
}

void test_writeLockUnlock(void)
{
	ReadWriteLock lock;
	lock.writeLock();
	lock.unlock();
}

void test_readFromMultipleThread(void)
{
	static const size_t NUM_THREAD = 3;
	pthread_t thread[NUM_THREAD];
	ReadThreadArg arg;
	arg.val = 0xaf57;
	arg.lock.readLock();

	for (size_t i = 0; i < NUM_THREAD; i++) {
		cppcut_assert_equal(0, pthread_create(&thread[i], NULL,
		                                      readThread, &arg));
	}
	for (size_t i = 0; i < NUM_THREAD; i++) {
		void *retVal;
		pthread_join(thread[i], &retVal);
		void *expected = reinterpret_cast<void *>(arg.val);
		cppcut_assert_equal(expected, retVal);
	}
}

void test_readFromMultipleThreadWithWriteLock(void)
{
	static const size_t NUM_THREAD = 3;
	pthread_t thread[NUM_THREAD];
	ReadThreadArg arg;
	arg.numNotReady = NUM_THREAD;
	arg.val = 0xaf57;
	arg.lock.writeLock();

	for (size_t i = 0; i < NUM_THREAD; i++) {
		cppcut_assert_equal(0, pthread_create(&thread[i], NULL,
		                                      readThread, &arg));
	}

	// wait for the ready of all threads
	while (__sync_fetch_and_add(&arg.numNotReady, 0) > 0)
		sched_yield();

	// change arg.val
	arg.val = 0xffaa;
	arg.lock.unlock();

	for (size_t i = 0; i < NUM_THREAD; i++) {
		void *retVal;
		pthread_join(thread[i], &retVal);
		void *expected = reinterpret_cast<void *>(arg.val);
		cppcut_assert_equal(expected, retVal);
	}
}

} // namespace testReadWriteLock
