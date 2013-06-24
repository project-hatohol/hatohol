/* Hatohol
   Copyright (C) 2013 MIRACLE LINUX CORPORATION
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <cstring>
#include <glib.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <Logger.h>
using namespace mlpl;

#include "Utils.h"
#include "FaceMySQL.h"
#include "FaceMySQLWorker.h"

static const int DEFAULT_PORT = 3306;

// ---------------------------------------------------------------------------
// Public methods
// ---------------------------------------------------------------------------
FaceMySQL::FaceMySQL(CommandLineArg &cmdArg)
: m_socket(NULL),
  m_port(DEFAULT_PORT)
{
	for (size_t i = 0; i < cmdArg.size(); i++) {
		string &cmd = cmdArg[i];
		if (cmd == "--face-mysql-port")
			i = parseCmdArgPort(cmdArg, i);
	}
	MLPL_INFO("started face-mysql, port: %d\n", m_port);
}

FaceMySQL::~FaceMySQL()
{
	if (m_socket)
		g_object_unref(m_socket);
}

// ---------------------------------------------------------------------------
// Protected methods
// ---------------------------------------------------------------------------
gpointer FaceMySQL::mainThread(HatoholThreadArg *arg)
{
	GError *error = NULL;
	m_socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
	                        G_SOCKET_PROTOCOL_TCP, &error);
	if (m_socket == NULL) {
		MLPL_ERR("Failed to create thread: %s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	struct sockaddr_in sockaddr;
	memset(&sockaddr, 0, sizeof(sockaddr));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(m_port);
	sockaddr.sin_addr.s_addr = INADDR_ANY;
	GSocketAddress *address =
	  g_socket_address_new_from_native(&sockaddr, sizeof(sockaddr));
	if (!address) {
		MLPL_ERR("Failed to call g_socket_address_new_from_native: "
		         "%s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	if (!g_socket_bind(m_socket, address, TRUE, &error)) {
		MLPL_ERR("Failed to call g_socket_bind: %s\n",
		         error->message);
		g_error_free(error);
		return NULL;
	}

	if (!g_socket_listen(m_socket, &error)) {
		MLPL_ERR("Failed to call g_socket_listen: %s\n",
		         error->message);
		g_error_free(error);
		return NULL;
	}

	uint32_t connId = 0;
	while (true) {
		// GSocket obtained here have to be freed in
		// the FaceMySQLWorker instance.
		GSocket *sock = g_socket_accept(m_socket, NULL, &error);
		if (sock == NULL) {
			MLPL_ERR("Failed to call g_socket_accept: %s\n",
			         error->message);
			g_error_free(error);
			error = NULL;
			continue;
		}
		FaceMySQLWorker *worker = new FaceMySQLWorker(sock, connId);
		worker->start(true);
		connId++;
	}

	return NULL;
}

// ---------------------------------------------------------------------------
// Private methods
// ---------------------------------------------------------------------------
size_t FaceMySQL::parseCmdArgPort(CommandLineArg &cmdArg, size_t idx)
{
	if (idx == cmdArg.size() - 1) {
		MLPL_ERR("needs port number.");
		return idx;
	}

	idx++;
	string &port_str = cmdArg[idx];
	int port = atoi(port_str.c_str());
	if (port < 0 || port > 65536) {
		MLPL_ERR("invalid port: %s, %d\n", port_str.c_str(), port);
		return idx;
	}

	m_port = port;
	return idx;
}
