/*
 * Copyright (C) 2013 Project Hatohol
 *
 * This file is part of Hatohol.
 *
 * Hatohol is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Hatohol is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Hatohol. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>
#include <cstdlib>
#include <glib.h>
#include "DBClientAction.h"

static gboolean printVersion(const gchar *optionName, const gchar *value,
                             gpointer data, GError **error)
{
	printf("%s\n", VERSION);
	exit(EXIT_SUCCESS);
	return TRUE;
}

static gboolean showActions(const gchar *optionName, const gchar *value,
                            gpointer data, GError **error)
{
	MLPL_BUG("Not implemented\n", __PRETTY_FUNCTION__);
	exit(EXIT_SUCCESS);
	return TRUE;
}

static const GOptionEntry optionEntries[] =
{
	{"version", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
	 (gpointer)printVersion, "Show version", NULL},

	{"show-action", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
	 (gpointer)showActions, "Show action definitions", NULL},

	{NULL}
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *optionCtx = g_option_context_new(NULL);
	g_option_context_add_main_entries(optionCtx, optionEntries, NULL);
	g_option_context_parse(optionCtx, &argc, &argv, &error);
	if (error) {
		fprintf(stderr, "Failed to parse argument: %s\n",
		        error->message);
		g_error_free(error);
		return EXIT_FAILURE;
	}
	
	g_option_context_free(optionCtx);
	return EXIT_SUCCESS;
}
