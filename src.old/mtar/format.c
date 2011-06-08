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
*  Last modified: Wed, 11 May 2011 13:59:16 +0200                       *
\***********************************************************************/

// free, realloc
#include <stdlib.h>
// strcmp
#include <string.h>

#include <mtar/format.h>

#include "loader.h"

static void mtar_format_exit(void);

static struct format {
	const char * name;
	mtar_format_f format;
} * mtar_format_formats = 0;
static unsigned int mtar_format_nbFormats = 0;


__attribute__((destructor))
void mtar_format_exit() {
	if (mtar_format_nbFormats > 0)
		free(mtar_format_formats);
	mtar_format_formats = 0;
}

struct mtar_format * mtar_format_get(struct mtar_io * io, const struct mtar_option * option) {
	unsigned int i;
	for (i = 0; i < mtar_format_nbFormats; i++) {
		if (!strcmp(option->format, mtar_format_formats[i].name))
			return mtar_format_formats[i].format(io, option);
	}
	if (mtar_loader_load("format", option->format))
		return 0;
	for (i = 0; i < mtar_format_nbFormats; i++) {
		if (!strcmp(option->format, mtar_format_formats[i].name))
			return mtar_format_formats[i].format(io, option);
	}
	return 0;
}

void mtar_format_register(const char * name, mtar_format_f f) {
	mtar_format_formats = realloc(mtar_format_formats, (mtar_format_nbFormats + 1) * (sizeof(struct format)));
	mtar_format_formats[mtar_format_nbFormats].name = name;
	mtar_format_formats[mtar_format_nbFormats].format = f;
	mtar_format_nbFormats++;

	mtar_loader_register_ok();
}

