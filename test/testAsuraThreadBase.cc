#include <cppcutter.h>
#include "AsuraThreadBase.h"
#include "AsuraException.h"

namespace testAsuraThreadBase {

typedef gpointer (*TestFunc)(void *data);

class Synchronizer {
	GMutex g_mutex;
public:
	Synchronizer(void)
	{
		g_mutex_init(&g_mutex);
	}

	virtual ~Synchronizer()
	{
		g_mutex_clear(&g_mutex);
	}

	void lock(void)
	{
		g_mutex_lock(&g_mutex);
	}

	void unlock(void)
	{
		g_mutex_unlock(&g_mutex);
	}

	gboolean trylock(void)
	{
		return g_mutex_trylock(&g_mutex);
	}


	void wait(void)
	{
		lock();
		unlock();
	}
};
static Synchronizer sync;

static gpointer throwAsuraExceptionFunc(void *data)
{
	THROW_ASURA_EXCEPTION("Test Exception");
	return NULL;
}

void exceptionCallbackFunc(const exception &e, void *data)
{
	sync.unlock();
}

class AsuraThreadTestImpl : public AsuraThreadBase {
public:
	AsuraThreadTestImpl(void)
	: m_testFunc(defaultTestFunc),
	  m_testData(NULL)
	{
	}

	bool doTest(void) {
		sync.lock();
		start();
		sync.wait();
		return true;
	}

	void setTestFunc(TestFunc func, void *data)
	{
		m_testFunc = func;
		m_testData = data;
	}

protected:
	virtual gpointer mainThread(AsuraThreadArg *arg)
	{
		return (*m_testFunc)(m_testData);
	}

	static gpointer defaultTestFunc(void *data)
	{
		return NULL;
	}

private:
	TestFunc m_testFunc;
	void    *m_testData;
};

void setup(void)
{
	if (!sync.trylock())
		cut_fail("lock is not unlocked.");
	sync.unlock();
}

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------
void test_exceptionCallback(void)
{
	AsuraThreadTestImpl threadTestee;
	threadTestee.setTestFunc(throwAsuraExceptionFunc, NULL);
	threadTestee.addExceptionCallback(exceptionCallbackFunc, NULL);
	cppcut_assert_equal(true, threadTestee.doTest());
}

} // namespace testAsuraThreadBase


