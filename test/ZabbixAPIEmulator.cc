#include <Logger.h>

#include "ZabbixAPIEmulator.h"

using namespace mlpl;

struct ZabbixAPIEmulator::PrivateContext {
	GThread *thread;
	
	// methods
	PrivateContext(void)
	: thread(NULL)
	{
	}

	virtual ~PrivateContext()
	{
	}
};

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
ZabbixAPIEmulator::ZabbixAPIEmulator(void)
: m_ctx(NULL)
{
	m_ctx = new PrivateContext();
}

ZabbixAPIEmulator::~ZabbixAPIEmulator()
{
	if (m_ctx)
		delete m_ctx;
}

bool ZabbixAPIEmulator::isRunning(void)
{
	return m_ctx->thread;
}

void ZabbixAPIEmulator::start(void)
{
	if (isRunning()) {
		MLPL_WARN("Thread is already running.");
		return;
	}
	
	m_ctx->thread = g_thread_new("ZabbixAPIEmulator", _mainThread, this);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer ZabbixAPIEmulator::_mainThread(gpointer data)
{
	ZabbixAPIEmulator *obj = static_cast<ZabbixAPIEmulator *>(data);
	return obj->mainThread();
}

gpointer ZabbixAPIEmulator::mainThread(void)
{
	return NULL;
}
