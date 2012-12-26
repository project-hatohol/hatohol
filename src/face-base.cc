#include <glib.h>

#include "utils.h"
#include "face-base.h"

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
face_base::face_base(void)
: m_thread(NULL)
{
}

face_base::~face_base()
{
	if (m_thread)
		g_thread_unref(m_thread);
}

void face_base::start(void)
{
	face_thread_arg_t *arg = new face_thread_arg_t();;
	arg->obj = this;
	m_thread = g_thread_new("face-base", thread_starter, arg);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
gpointer face_base::thread_starter(gpointer data)
{
	face_thread_arg_t *arg = static_cast<face_thread_arg_t *>(data);
	gpointer ret = arg->obj->main_thread(arg);
	delete arg;
	return ret;
}
