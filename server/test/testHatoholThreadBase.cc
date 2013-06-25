#include <cppcutter.h>
#include "HatoholThreadBase.h"
#include "HatoholException.h"
#include "Synchronizer.h"

namespace testHatoholThreadBase {

typedef gpointer (*TestFunc)(void *data);
static Synchronizer sync;

static gpointer throwHatoholExceptionFunc(void *data)
{
	THROW_HATOHOL_EXCEPTION("Test Exception");
	return NULL;
}

void exceptionCallbackFunc(const exception &e, void *data)
{
	sync.unlock();
}

void exitCallbackFunc(void *data)
{
	sync.unlock();
}

class HatoholThreadTestImpl : public HatoholThreadBase {
public:
	HatoholThreadTestImpl(void)
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
	virtual gpointer mainThread(HatoholThreadArg *arg)
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
	HatoholThreadTestImpl threadTestee;
	threadTestee.setTestFunc(throwHatoholExceptionFunc, NULL);
	threadTestee.addExceptionCallback(exceptionCallbackFunc, NULL);
	cppcut_assert_equal(true, threadTestee.doTest());
}

void test_exitCallback(void)
{
	HatoholThreadTestImpl threadTestee;
	threadTestee.addExitCallback(exitCallbackFunc, NULL);
	cppcut_assert_equal(true, threadTestee.doTest());
}

} // namespace testHatoholThreadBase


