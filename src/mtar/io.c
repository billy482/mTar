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
*  Last modified: Thu, 21 Apr 2011 10:05:59 +0200                       *
\***********************************************************************/

// errno
#include <errno.h>
// realloc
#include <stdlib.h>
// strerror, strcmp
#include <string.h>
// stat
#include <sys/stat.h>
// stat
#include <sys/types.h>
// access, stat
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "io.h"
#include "loader.h"

static struct io {
	const char * name;
	mtar_io_f function;
} * ios = 0;
static unsigned int nbIos = 0;

static struct mtar_io * io_get(const char * io, struct mtar_option * option);
static int io_isReadable(mtar_function_enum function);
static int io_isWritable(mtar_function_enum function);


struct mtar_io * io_get(const char * io, struct mtar_option * option) {
	unsigned int i;
	for (i = 0; i < nbIos; i++) {
		if (!strcmp(io, ios[i].name))
			return ios[i].function(option);
	}
	if (loader_load("io", io))
		return 0;
	for (i = 0; i < nbIos; i++) {
		if (!strcmp(io, ios[i].name))
			return ios[i].function(option);
	}
	return 0;
}

int io_isReadable(mtar_function_enum function) {
	switch (function) {
		default:
			return 0;
	}
}

int io_isWritable(mtar_function_enum function) {
	switch (function) {
		case MTAR_FUNCTION_CREATE:
			return 1;

		default:
			return 0;
	}
}


struct mtar_io * mtar_io_get(struct mtar_option * option) {
	if (option->filename) {
		int mode = 0;
		if (access(option->filename, F_OK))
			return io_get("file", option);

		if (io_isWritable(option->function))
			mode = F_OK | W_OK;
		else if (io_isReadable(option->function))
			mode = F_OK | R_OK;

		if (access(option->filename, mode)) {
			mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Access to file (%s) failed => %s\n", option->filename, strerror(errno));
			return 0;
		}

		struct stat st;
		if (stat(option->filename, &st)) {
			mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Getting information about file (%s) failed => %s\n", option->filename, strerror(errno));
			return 0;
		}

		if (S_ISREG(st.st_mode)) {
			return io_get("file", option);
		}
	}

	return 0;
}

void mtar_io_register(const char * name, mtar_io_f function) {
	ios = realloc(ios, (nbIos + 1) * sizeof(struct io));
	ios[nbIos].name = name;
	ios[nbIos].function = function;
	nbIos++;

	loader_register_ok();
}

