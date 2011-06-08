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
*  Last modified: Wed, 11 May 2011 13:59:04 +0200                       *
\***********************************************************************/

// errno
#include <errno.h>
// open
#include <fcntl.h>
// realloc
#include <stdlib.h>
// strerror, strcmp
#include <string.h>
// open, stat
#include <sys/stat.h>
// open, stat
#include <sys/types.h>
// access, stat
#include <unistd.h>

#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "loader.h"

static struct io {
	const char * name;
	mtar_io_f function;
} * mtar_io_ios = 0;
static unsigned int mtar_io_nbIos = 0;

static struct mtar_io * mtar_io_io_get(int fd, int flags, const char * io, const struct mtar_option * option);
static void mtar_io_exit(void);


struct mtar_io * mtar_io_io_get(int fd, int flags, const char * io, const struct mtar_option * option) {
	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++) {
		if (!strcmp(io, mtar_io_ios[i].name))
			return mtar_io_ios[i].function(fd, flags, option);
	}
	if (mtar_loader_load("io", io))
		return 0;
	for (i = 0; i < mtar_io_nbIos; i++) {
		if (!strcmp(io, mtar_io_ios[i].name))
			return mtar_io_ios[i].function(fd, flags, option);
	}
	return 0;
}


struct mtar_io * mtar_io_get_fd(int fd, int flags, const struct mtar_option * option) {
	struct stat st;

	if (fstat(fd, &st)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Getting information about file descriptor (%d) failed => %s\n", fd, strerror(errno));
		return 0;
	}

	if (S_ISREG(st.st_mode))
		return mtar_io_io_get(fd, flags, "file", option);
	else if (S_ISFIFO(st.st_mode))
		return mtar_io_io_get(fd, flags, "pipe", option);

	return 0;
}

struct mtar_io * mtar_io_get_file(const char * filename, int flags, const struct mtar_option * option) {
	int m = F_OK;
	if (flags & O_RDWR)
		m |= R_OK | W_OK;
	else if (flags & O_RDONLY)
		m |= R_OK;
	else if (flags & O_WRONLY)
		m |= W_OK;

	if (access(filename, m))
		flags |= O_CREAT;

	int fd = open(filename, flags, 0644);
	if (fd < 0) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Failed to open (%s) => %s\n", filename, strerror(errno));
		return 0;
	}

	return mtar_io_get_fd(fd, flags, option);
}

void mtar_io_register(const char * name, mtar_io_f function) {
	mtar_io_ios = realloc(mtar_io_ios, (mtar_io_nbIos + 1) * sizeof(struct io));
	mtar_io_ios[mtar_io_nbIos].name = name;
	mtar_io_ios[mtar_io_nbIos].function = function;
	mtar_io_nbIos++;

	mtar_loader_register_ok();
}

__attribute__((destructor))
void mtar_io_exit() {
	if (mtar_io_nbIos > 0)
		free(mtar_io_ios);
	mtar_io_ios = 0;
}

