#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include <inttypes.h>

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
	int exitCode;

	PrivateContext(void)
	: loop(NULL),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  exitCode(EXIT_SUCCESS)
	{
	}

	virtual ~PrivateContext()
	{
		if (loop)
			g_main_loop_unref(loop);
	}
};

static void requestQuit(PrivateContext *ctx, int exitCode = EXIT_FAILURE)
{
	ctx->exitCode = exitCode;
	g_main_loop_quit(ctx->loop);
}

gboolean readPipeCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(ctx);
	return TRUE;
}

gboolean writePipeCb(GIOChannel *source, GIOCondition condition, gpointer data)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(ctx);
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

static void getParametersBodyCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                     size_t size, void *priv)
{
	// length of the module path
	sbuf.resetIndex();
	uint16_t modulePathLen = *sbuf.getPointerAndIncIndex<uint16_t>();
	string modulePath(sbuf.getPointer<char>(), modulePathLen);
	sbuf.incIndex(modulePathLen);
	MLPL_BUG("Not implemented: %s, %s\n", __PRETTY_FUNCTION__,
	         modulePath.c_str());
	// TODO: load the module and notify the result
}

static void getParametersCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                            size_t size, void *priv)
{
	PrivateContext *ctx = static_cast<PrivateContext *>(priv);

	// check the packet
	sbuf.resetIndex();
	uint32_t pktLen = *sbuf.getPointerAndIncIndex<uint32_t>();

	// packet type
	uint16_t pktType = *sbuf.getPointerAndIncIndex<uint16_t>();
	if (pktType != RESIDENT_PROTO_PKT_TYPE_PARAMETERS) {
		MLPL_ERR("Invalid packet type: %"PRIu16", "
		         "expect: %"PRIu16"\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		requestQuit(ctx);
	}

	// request the remaining part
	size_t bodyLen = pktLen - RESIDENT_PROTO_HEADER_PKT_TYPE_LEN;
	ctx->pipeRd.pull(bodyLen, getParametersBodyCb, ctx);
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

	return ctx.exitCode;
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
