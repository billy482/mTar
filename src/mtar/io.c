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
*  Last modified: Sun, 17 Apr 2011 21:08:33 +0200                       *
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

#include "io.h"
#include "loader.h"
#include "verbose.h"

static struct io {
	const char * name;
	struct mtar_io * (*function)(struct mtar_option * option);
} * ios = 0;
unsigned int nbIos = 0;

static struct mtar_io * io_get(const char * io, struct mtar_option * option);
static int io_isWritable(enum mtar_function function);


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

int io_isWritable(enum mtar_function function) {
	switch (function) {
		case MTAR_CREATE:
			return 1;

		default:
			return 0;
	}
}


struct mtar_io * mtar_io_get(struct mtar_option * option) {
	if (option->filename) {
		int mode = 0;
		if (io_isWritable(option->function))
			mode = F_OK | W_OK;

		if (access(option->filename, mode)) {
			mtar_verbose_printf("Access to file (%s) failed => %s\n", option->filename, strerror(errno));
			return 0;
		}

		struct stat st;
		if (stat(option->filename, &st)) {
			mtar_verbose_printf("Getting information about file (%s) failed => %s\n", option->filename, strerror(errno));
			return 0;
		}

		if (S_ISREG(st.st_mode)) {
			return io_get("file", option);
		}
	}

	return 0;
}

void mtar_io_register(const char * name, struct mtar_io * (*io)(struct mtar_option * option)) {
	ios = realloc(ios, (nbIos + 1) * sizeof(struct io));
	ios[nbIos].name = name;
	ios[nbIos].function = io;
	nbIos++;

	loader_register_ok();
}

