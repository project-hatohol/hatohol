/*
 * Copyright (C) 2014 Project Hatohol
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

#include <gcutter.h>
#include "DataSamples.h"

void addDataSamplesForGCutBool(void)
{
	gcut_add_datum("False",
	               "val", G_TYPE_BOOLEAN, FALSE,
	               NULL);
	gcut_add_datum("True",
	               "val", G_TYPE_BOOLEAN, TRUE,
	               NULL);
}

void addDataSamplesForGCutInt(void)
{
	gcut_add_datum("Zero",
	               "val", G_TYPE_INT, (gint)0,
	               "expect", G_TYPE_STRING, "0",
	               NULL);
	gcut_add_datum("Positive within 32bit",
	               "val", G_TYPE_INT, (gint)3456,
	               "expect", G_TYPE_STRING, "3456",
	               NULL);
	gcut_add_datum("Positive 32bit Max",
	               "val", G_TYPE_INT, (gint)2147483647,
	               "expect", G_TYPE_STRING, "2147483647",
	               NULL);
	gcut_add_datum("Negative within 32bit",
	               "val", G_TYPE_INT, (gint)-1389,
	               "expect", G_TYPE_STRING, "-1389",
	               NULL);
	gcut_add_datum("Negative 32bit Min",
	               "val", G_TYPE_INT, (gint)-2147483648,
	               "expect", G_TYPE_STRING, "-2147483648",
	               NULL);
}

void addDataSamplesForGCutUint64(void)
{
	gcut_add_datum("Zero",
	               "val", G_TYPE_UINT64, (guint64)0,
	               "expect", G_TYPE_STRING, "0",
	               NULL);
	gcut_add_datum("Positive within 32bit",
	               "val", G_TYPE_UINT64, (guint64)3456,
	               "expect", G_TYPE_STRING, "3456",
	               NULL);
	gcut_add_datum("Positive 32bit Max",
	               "val", G_TYPE_UINT64, (guint64)2147483647,
	               "expect", G_TYPE_STRING, "2147483647",
	               NULL);
	gcut_add_datum("Positive 32bit Max + 1",
	               "val", G_TYPE_UINT64, (guint64)2147483648,
	               "expect", G_TYPE_STRING, "2147483648",
	               NULL);
	gcut_add_datum("Positive 64bit Poistive Max",
	               "val", G_TYPE_UINT64, 9223372036854775807UL,
	               "expect", G_TYPE_STRING, "9223372036854775807",
	               NULL);
	gcut_add_datum("Positive 64bit Poistive Max+1",
	               "val", G_TYPE_UINT64, 9223372036854775808UL,
	               "expect", G_TYPE_STRING, "-9223372036854775808",
	               NULL);
	gcut_add_datum("Positive 64bit Max",
	               "val", G_TYPE_UINT64, 18446744073709551615UL,
	               "expect", G_TYPE_STRING, "-1",
	               NULL);
}
