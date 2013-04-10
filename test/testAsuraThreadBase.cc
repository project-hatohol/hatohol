#include <cppcutter.h>
#include "AsuraThreadBase.h"
#include "AsuraException.h"
#include "Synchronizer.h"

namespace testAsuraThreadBase {

typedef gpointer (*TestFunc)(void *data);
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


