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
*  Last modified: Thu, 21 Apr 2011 22:12:15 +0200                       *
\***********************************************************************/

// strcmp
#include <string.h>
// realloc
#include <stdlib.h>

#include <mtar/format.h>

#include "loader.h"

static struct format {
	const char * name;
	mtar_format_f format;
} * formats = 0;
static unsigned int nbFormats = 0;


struct mtar_format * mtar_format_get(struct mtar_io * io, struct mtar_option * option) {
	unsigned int i;
	for (i = 0; i < nbFormats; i++) {
		if (!strcmp(option->format, formats[i].name))
			return formats[i].format(io, option);
	}
	if (loader_load("format", option->format))
		return 0;
	for (i = 0; i < nbFormats; i++) {
		if (!strcmp(option->format, formats[i].name))
			return formats[i].format(io, option);
	}
	return 0;
}

void mtar_format_register(const char * name, mtar_format_f f) {
	formats = realloc(formats, (nbFormats + 1) * (sizeof(struct format)));
	formats[nbFormats].name = name;
	formats[nbFormats].format = f;
	nbFormats++;

	loader_register_ok();
}

