#include <glib.h>

#include <Logger.h>
using namespace mlpl;

#include "Utils.h"
#include "FaceBase.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceBase::FaceBase(void)
: m_thread(NULL)
{
}

FaceBase::~FaceBase()
{
	if (m_thread)
		g_thread_unref(m_thread);
}

void FaceBase::start(void)
{
	FaceThreadArg *arg = new FaceThreadArg();;
	arg->obj = this;
	GError *error = NULL;
	m_thread = g_thread_try_new("face-base", threadStarter, arg, &error);
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
gpointer FaceBase::threadStarter(gpointer data)
{
	FaceThreadArg *arg = static_cast<FaceThreadArg *>(data);
	gpointer ret = arg->obj->mainThread(arg);
	delete arg;
	return ret;
}
