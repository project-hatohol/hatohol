#include <cstdlib>
#include <glib.h>

#include <Logger.h>
using namespace mlpl;

#include "Hatohol.h"
#include "HatoholException.h"

struct PrivateContext {
	GMainLoop *loop;

	PrivateContext(void)
	: loop(NULL)
	{
	}

	virtual ~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
	}
};

int mainRoutine(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32 

	hatoholInit();
	MLPL_INFO("started hatohol-resident-yard: ver. %s\n", PACKAGE_VERSION);

	// main loop of GLIB
	PrivateContext ctx;
	ctx.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(ctx.loop);

	return EXIT_SUCCESS;
}


int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;
	try {
		ret = mainRoutine(argc, argv);
	} catch (const HatoholException &e){
		MLPL_ERR("Got exception: %s", e.getFancyMessage().c_str());
	} catch (const exception &e) {
		MLPL_ERR("Got exception: %s", e.what());
	}
	return ret;
}
