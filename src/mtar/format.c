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
*  Last modified: Mon, 18 Apr 2011 23:16:51 +0200                       *
\***********************************************************************/

// realloc
#include <stdlib.h>

#include <mtar/format.h>
#include <mtar/option.h>

#include "loader.h"

static struct format {
	const char * name;
	mtar_format_f format;
} * formats = 0;
static int nbFormats = 0;


struct mtar_format * mtar_format_get(struct mtar_io * io, struct mtar_option * option, struct mtar_verbose * verbose) {
	switch (option->format) {
		default:
			return 0;
	}
}

void mtar_format_register(const char * name, mtar_format_f f) {
	formats = realloc(formats, (nbFormats + 1) * (sizeof(struct format)));
	formats[nbFormats].name = name;
	formats[nbFormats].format = f;
	nbFormats++;

	loader_register_ok();
}

