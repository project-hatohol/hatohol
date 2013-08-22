#include <cstdio>
#include <cstdlib>
#include <glib.h>

#include <Logger.h>
#include <SmartBuffer.h>
using namespace mlpl;

#include "Hatohol.h"
#include "HatoholException.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"

struct PrivateContext {
	GMainLoop *loop;
	NamedPipe pipeRd;
	NamedPipe pipeWr;

	PrivateContext(void)
	: loop(NULL),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE)
	{
	}

	virtual ~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
	}
};

gboolean readPipeCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_BUG("Not implemented: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	return TRUE;
}

gboolean writePipeCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_BUG("Not implemented: %s (%p)\n", __PRETTY_FUNCTION__, ctx);
	return TRUE;
}

static void sendLaunched(PrivateContext *ctx)
{
	SmartBuffer buf(RESIDENT_PROTO_HEADER_LEN);
	buf.add32(
	  RESIDENT_PROTO_HEADER_LEN - RESIDENT_PROTO_HEADER_PKT_SIZE_LEN);
	buf.add16(RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
	ctx->pipeWr.push(buf);
}

void getParametersCb(GIOStatus stat, mlpl::SmartBuffer &buf,
                     size_t size, void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);
	MLPL_BUG("Not implemented: %s, %p\n", __PRETTY_FUNCTION__, ctx);
}

static void setupGetParametersCb(PrivateContext *ctx)
{
	ctx->pipeRd.pull(RESIDENT_PROTO_HEADER_LEN, getParametersCb, ctx);
}

int mainRoutine(int argc, char *argv[])
{
#ifndef GLIB_VERSION_2_36
	g_type_init();
#endif // GLIB_VERSION_2_36
#ifndef GLIB_VERSION_2_32
	g_thread_init(NULL);
#endif // GLIB_VERSION_2_32 

	hatoholInit();
	PrivateContext ctx;
	MLPL_INFO("started hatohol-resident-yard: ver. %s\n", PACKAGE_VERSION);

	// open pipes
	if (argc < 2) {
		MLPL_ERR("The pipe name is not given. (%d)\n", argc);
		return EXIT_FAILURE;
	}
	const char *pipeName = argv[1];
	MLPL_INFO("PIPE name: %s\n", pipeName);
	if (!ctx.pipeRd.init(pipeName,readPipeCb, &ctx))
		return EXIT_FAILURE;
	if (!ctx.pipeWr.init(pipeName, writePipeCb, &ctx))
		return EXIT_FAILURE;

	sendLaunched(&ctx);
	setupGetParametersCb(&ctx);

	// main loop of GLIB
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
