/***********************************************************************\
*                               ______                                  *
*                          __ _/_  __/__ _____                          *
*                         /  ' \/ / / _ `/ __/                          *
*                        /_/_/_/_/  \_,_/_/                             *
*                                                                       *
*  -------------------------------------------------------------------  *
*  This file is a part of mTar                                          *
*                                                                       *
*  mTar is free software; you can redistribute it and/or                *
*  modify it under the terms of the GNU General Public License          *
*  as published by the Free Software Foundation; either version 3       *
*  of the License, or (at your option) any later version.               *
*                                                                       *
*  This program is distributed in the hope that it will be useful,      *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*  GNU General Public License for more details.                         *
*                                                                       *
*  You should have received a copy of the GNU General Public License    *
*  along with this program; if not, write to the Free Software          *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
*  Boston, MA  02110-1301, USA.                                         *
*                                                                       *
*  -------------------------------------------------------------------  *
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>  *
*  Last modified: Thu, 16 Jun 2011 09:51:25 +0200                       *
\***********************************************************************/

// free, realloc
#include <stdlib.h>
// strcmp
#include <string.h>

#include "format.h"
#include "loader.h"

static void mtar_format_exit(void);
static struct mtar_format * mtar_format_get(const char * name);

static struct mtar_format ** mtar_format_formats = 0;
static unsigned int mtar_format_nbFormats = 0;


__attribute__((destructor))
void mtar_format_exit() {
	if (mtar_format_nbFormats > 0)
		free(mtar_format_formats);
	mtar_format_formats = 0;
}

struct mtar_format * mtar_format_get(const char * name) {
	unsigned int i;
	for (i = 0; i < mtar_format_nbFormats; i++) {
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];
	}
	if (mtar_loader_load("format", name))
		return 0;
	for (i = 0; i < mtar_format_nbFormats; i++) {
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];
	}
	return 0;
}

struct mtar_format_in * mtar_format_get_in(struct mtar_io_in * io, const struct mtar_option * option) {
	struct mtar_format * format = mtar_format_get(option->format);
	if (format)
		return format->newIn(io, option);
	return 0;
}

struct mtar_format_out * mtar_format_get_out(struct mtar_io_out * io, const struct mtar_option * option) {
	struct mtar_format * format = mtar_format_get(option->format);
	if (format)
		return format->newOut(io, option);
	return 0;
}

void mtar_format_register(struct mtar_format * f) {
	mtar_format_formats = realloc(mtar_format_formats, (mtar_format_nbFormats + 1) * (sizeof(struct mtar_format *)));
	mtar_format_formats[mtar_format_nbFormats] = f;
	mtar_format_nbFormats++;

	mtar_loader_register_ok();
}

void mtar_format_showDescription() {
	mtar_loader_loadAll("format");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\nList of available formats :\n");

	unsigned int i;
	for (i = 0; i < mtar_format_nbFormats; i++)
		mtar_format_formats[i]->showDescription();
}

