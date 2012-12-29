#include <glib.h>

#include <Logger.h>
using namespace mlpl;

#include "Utils.h"
#include "AsuraThreadBase.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
AsuraThreadBase::AsuraThreadBase(void)
: m_thread(NULL)
{
}

AsuraThreadBase::~AsuraThreadBase()
{
	if (m_thread)
		g_thread_unref(m_thread);
}

void AsuraThreadBase::start(void)
{
	AsuraThreadArg *arg = new AsuraThreadArg();;
	arg->obj = this;
	GError *error = NULL;
	m_thread = g_thread_try_new("AsuraThread", threadStarter, arg, &error);
	if (m_thread == NULL) {
		MLPL_ERR("Failed to call g_thread_try_new: %s\n",
		         error->message);
	}
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer AsuraThreadBase::threadStarter(gpointer data)
{
	AsuraThreadArg *arg = static_cast<AsuraThreadArg *>(data);
	gpointer ret = arg->obj->mainThread(arg);
	delete arg;
	return ret;
}
