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
*  Last modified: Tue, 05 Jul 2011 07:19:21 +0200                       *
\***********************************************************************/

// free, realloc
#include <stdlib.h>
// bzero, strcmp
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
		return format->new_in(io, option);
	return 0;
}

struct mtar_format_out * mtar_format_get_out(struct mtar_io_out * io, const struct mtar_option * option) {
	struct mtar_format * format = mtar_format_get(option->format);
	if (format)
		return format->new_out(io, option);
	return 0;
}

void mtar_format_init_header(struct mtar_format_header * h) {
	if (!h)
		return;

	h->dev = 0;
	bzero(h->path, 256);
	h->filename = 0;
	bzero(h->link, 256);
	h->size = 0;
	h->mode = 0;
	h->mtime = 0;
	h->uid = 0;
	bzero(h->uname, 32);
	h->gid = 0;
	bzero(h->gname, 32);
}

void mtar_format_register(struct mtar_format * f) {
	mtar_format_formats = realloc(mtar_format_formats, (mtar_format_nbFormats + 1) * (sizeof(struct mtar_format *)));
	mtar_format_formats[mtar_format_nbFormats] = f;
	mtar_format_nbFormats++;

	mtar_loader_register_ok();
}

void mtar_format_show_description() {
	mtar_loader_loadAll("format");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\nList of available formats :\n");

	unsigned int i;
	for (i = 0; i < mtar_format_nbFormats; i++)
		mtar_format_formats[i]->show_description();
}

