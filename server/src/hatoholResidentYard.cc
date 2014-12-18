/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3
 * as published by the Free Software Foundation.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Hatohol. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <glib-object.h>
#include <inttypes.h>
#include <dlfcn.h>
#include <Logger.h>
#include <SmartBuffer.h>
#include "Hatohol.h"
#include "HatoholException.h"
#include "NamedPipe.h"
#include "ResidentProtocol.h"
#include "ResidentCommunicator.h"
using namespace std;
using namespace mlpl;

struct Impl : public ResidentPullHelper<Impl> {
	GMainLoop *loop;
	NamedPipe pipeRd;
	NamedPipe pipeWr;
	int exitCode;
	void *moduleHandle;
	ResidentModule *module;

	Impl(void)
	: loop(NULL),
	  pipeRd(NamedPipe::END_TYPE_SLAVE_READ),
	  pipeWr(NamedPipe::END_TYPE_SLAVE_WRITE),
	  exitCode(EXIT_SUCCESS),
	  moduleHandle(NULL)
	{
		initResidentPullHelper(&pipeRd, this);
	}

	virtual ~Impl()
	{
		if (loop)
			g_main_loop_unref(loop);
		if (moduleHandle) {
			if (dlclose(moduleHandle) != 0)
				MLPL_ERR("Failed to close module.");
		}
	}
};

static void requestQuit(Impl *impl, int exitCode = EXIT_FAILURE)
{
	impl->exitCode = exitCode;
	g_main_loop_quit(impl->loop);
}

static gboolean readPipeCb
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	Impl *impl = static_cast<Impl *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(impl);
	return TRUE;
}

static gboolean writePipeCb
  (GIOChannel *source, GIOCondition condition, gpointer data)
{
	Impl *impl = static_cast<Impl *>(data);
	MLPL_ERR("Error: condition: %x\n", condition);
	requestQuit(impl);
	return TRUE;
}

static void eventCb(GIOStatus stat, SmartBuffer &sbuf, size_t size,
                    Impl *impl);

static void gotNotifyEventBodyCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                 size_t size, Impl *impl)
{
	ResidentNotifyEventArg arg;
	arg.actionId        = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.serverId        = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.hostId          = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.time.tv_sec     = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.time.tv_nsec    = *sbuf.getPointerAndIncIndex<uint32_t>();
	arg.eventId         = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.eventType       = *sbuf.getPointerAndIncIndex<uint16_t>();
	arg.triggerId       = *sbuf.getPointerAndIncIndex<uint64_t>();
	arg.triggerStatus   = *sbuf.getPointerAndIncIndex<uint16_t>();
	arg.triggerSeverity = *sbuf.getPointerAndIncIndex<uint16_t>();
	memcpy(arg.sessionId, sbuf.getPointer<char>(), HATOHOL_SESSION_ID_LEN);
	arg.sessionId[HATOHOL_SESSION_ID_LEN] = '\0';  // NULL terminator
	sbuf.incIndex(HATOHOL_SESSION_ID_LEN);

	// call a user action
	uint32_t resultCode = (*impl->module->notifyEvent)(&arg);
	ResidentCommunicator comm;
	comm.setNotifyEventAck(resultCode);
	comm.push(impl->pipeWr);

	// request to get the envet
	impl->pullHeader(eventCb);
}

static void eventCb(GIOStatus stat, SmartBuffer &sbuf, size_t size,
                    Impl *impl)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		requestQuit(impl);
		return;
	}

	int pktType = ResidentCommunicator::getPacketType(sbuf);
	if (pktType == RESIDENT_PROTO_PKT_TYPE_NOTIFY_EVENT) {
		// request to get the body
		impl->pullData(RESIDENT_PROTO_EVENT_BODY_LEN,
		              gotNotifyEventBodyCb);
	} else {
		MLPL_ERR("Unexpected packet: %d\n", pktType);
		requestQuit(impl);
		return;
	}
}

static void sendLaunched(Impl *impl)
{
	ResidentCommunicator comm;
	comm.setHeader(0, RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
	comm.push(impl->pipeWr);
}

static void sendModuleLoaded(Impl *impl, uint32_t code)
{
	ResidentCommunicator comm;
	comm.setModuleLoaded(code);
	comm.push(impl->pipeWr);
}

static void getParametersBodyCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                                size_t size, Impl *impl)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		return;
	}

	// Get the module path and the option
	uint16_t modulePathLen = *sbuf.getPointerAndIncIndex<uint16_t>();
	string modulePath(sbuf.getPointer<char>(), modulePathLen);
	sbuf.incIndex(modulePathLen);

	uint16_t moduleOptionLen = *sbuf.getPointerAndIncIndex<uint16_t>();
	string moduleOption(sbuf.getPointer<char>(), moduleOptionLen);
	sbuf.incIndex(moduleOptionLen);

	// open the module
	impl->moduleHandle = dlopen(modulePath.c_str(), RTLD_LAZY);
	if (!impl->moduleHandle) {
		MLPL_ERR("Failed to load module: %p, %s\n",
		         modulePath.c_str(), dlerror());
		sendModuleLoaded(
		  impl, RESIDENT_PROTO_MODULE_LOADED_CODE_FAIL_DLOPEN);
		return;
	}

	// get the address of the information structure
	dlerror(); // Clear any existing error
	impl->module = (ResidentModule *)
	  dlsym(impl->moduleHandle, RESIDENT_MODULE_SYMBOL_STR);
	char *error;
	if ((error = dlerror()) != NULL) { MLPL_ERR("Failed to load symbol: %s, %s\n",
		         RESIDENT_MODULE_SYMBOL_STR, error);
		sendModuleLoaded(
		  impl, RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_MOD_SYMBOL);
		return;
	}

	// check the module version
	if (impl->module->moduleVersion != RESIDENT_MODULE_VERSION) {
		MLPL_ERR("Module version unmatched: %" PRIu16 ", "
		         "expected: %" PRIu16 "\n",
		         impl->module->moduleVersion, RESIDENT_MODULE_VERSION);
		sendModuleLoaded(
		  impl, RESIDENT_PROTO_MODULE_LOADED_CODE_MOD_VER_INVALID);
		return;
	}

	// check init
	if (impl->module->init) {
		uint32_t result = (*impl->module->init)(moduleOption.c_str());
		if (result != RESIDENT_MOD_INIT_OK) {
			MLPL_ERR("Failed to initialize: %" PRIu32 "\n", result);
			sendModuleLoaded(
			  impl, RESIDENT_PROTO_MODULE_LOADED_CODE_INIT_FAILURE);
			return;
		}
	} else if (!moduleOption.empty()) {
		MLPL_WARN("Init. handler is NULL, but module option is "
		          "specified: %s\n", moduleOption.c_str());
	}

	// check functions
	if (!impl->module->notifyEvent) {
		MLPL_ERR("notify Event handler is NULL\n");
		sendModuleLoaded(
		  impl,
		  RESIDENT_PROTO_MODULE_LOADED_CODE_NOT_FOUND_NOTIFY_EVENT);
		return;
	}

	MLPL_INFO("Loaded a resident module: %s\n", modulePath.c_str());

	// send a completion notify
	sendModuleLoaded(impl, RESIDENT_PROTO_MODULE_LOADED_CODE_SUCCESS);

	// request to get the envet
	impl->pullHeader(eventCb);
}

static void getParametersCb(GIOStatus stat, mlpl::SmartBuffer &sbuf,
                            size_t size, Impl *impl)
{
	if (stat != G_IO_STATUS_NORMAL) {
		MLPL_ERR("Error: status: %x\n", stat);
		return;
	}

	// check the packet
	uint32_t bodyLen = *sbuf.getPointerAndIncIndex<uint32_t>();

	// packet type
	uint16_t pktType = *sbuf.getPointerAndIncIndex<uint16_t>();
	if (pktType != RESIDENT_PROTO_PKT_TYPE_PARAMETERS) {
		MLPL_ERR("Invalid packet type: %" PRIu16 ", "
		         "expect: %" PRIu16 "\n", pktType,
		         RESIDENT_PROTO_PKT_TYPE_LAUNCHED);
		requestQuit(impl);
		return;
	}

	// request to get the body
	impl->pullData(bodyLen, getParametersBodyCb);
}

static void setupGetParametersCb(Impl *impl)
{
	impl->pullHeader(getParametersCb);
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
	Impl impl;
	MLPL_INFO("started hatohol-resident-yard: ver. %s\n", PACKAGE_VERSION);

	// open pipes
	if (argc < 2) {
		MLPL_ERR("The pipe name is not given. (%d)\n", argc);
		return EXIT_FAILURE;
	}
	const char *pipeName = argv[1];
	MLPL_INFO("PIPE name: %s\n", pipeName);
	if (!impl.pipeRd.init(pipeName,readPipeCb, &impl))
		return EXIT_FAILURE;
	if (!impl.pipeWr.init(pipeName, writePipeCb, &impl))
		return EXIT_FAILURE;

	sendLaunched(&impl);
	setupGetParametersCb(&impl);

	// main loop of GLIB
	impl.loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(impl.loop);

	return impl.exitCode;
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
