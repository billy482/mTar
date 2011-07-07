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
*  Last modified: Wed, 06 Jul 2011 17:39:34 +0200                       *
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

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "io.h"
#include "loader.h"

static struct mtar_io ** mtar_io_ios = 0;
static unsigned int mtar_io_nbIos = 0;

static void mtar_io_exit(void) __attribute__((destructor));
struct mtar_io * mtar_io_get(int fd);
int mtar_io_open(const char * filename, int flags);


void mtar_io_exit() {
	if (mtar_io_nbIos > 0)
		free(mtar_io_ios);
	mtar_io_ios = 0;
}

struct mtar_io * mtar_io_get(int fd) {
	struct stat st;

	if (fstat(fd, &st)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Getting information about file descriptor (%d) failed => %s\n", fd, strerror(errno));
		return 0;
	}

	const char * module = 0;
	if (S_ISREG(st.st_mode))
		module = "file";
	else if (S_ISFIFO(st.st_mode))
		module = "pipe";

	if (!module)
		return 0;

	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		if (!strcmp(module, mtar_io_ios[i]->name))
			return mtar_io_ios[i];
	if (mtar_loader_load("io", module))
		return 0;
	for (i = 0; i < mtar_io_nbIos; i++)
		if (!strcmp(module, mtar_io_ios[i]->name))
			return mtar_io_ios[i];
	return 0;
}

struct mtar_io_in * mtar_io_in_get_fd(int fd, int flags, const struct mtar_option * option) {
	struct mtar_io * io = mtar_io_get(fd);
	if (io)
		return io->new_in(fd, flags, option);

	return 0;
}

struct mtar_io_in * mtar_io_in_get_file(const char * filename, int flags, const struct mtar_option * option) {
	int fd = mtar_io_open(filename, flags);
	if (fd < 0)
		return 0;

	return mtar_io_in_get_fd(fd, flags, option);;
}

int mtar_io_open(const char * filename, int flags) {
	int m = F_OK;
	if (flags & O_RDWR)
		m |= R_OK | W_OK;
	else if (flags & O_RDONLY)
		m |= R_OK;
	else if (flags & O_WRONLY)
		m |= W_OK;

	if (!strcmp("-", filename)) {
		if (flags & O_RDONLY)
			return 0;
		if (flags & O_WRONLY)
			return 1;
	}

	if (access(filename, m))
		flags |= O_CREAT;

	int fd = open(filename, flags, 0644);
	if (fd < 0)
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Failed to open (%s) => %s\n", filename, strerror(errno));

	return fd;
}

struct mtar_io_out * mtar_io_out_get_fd(int fd, int flags, const struct mtar_option * option) {
	struct mtar_io * io = mtar_io_get(fd);
	if (io)
		return io->new_out(fd, flags, option);

	return 0;
}

struct mtar_io_out * mtar_io_out_get_file(const char * filename, int flags, const struct mtar_option * option) {
	int fd = mtar_io_open(filename, flags);
	if (fd < 0)
		return 0;

	return mtar_io_out_get_fd(fd, flags, option);
}

void mtar_io_register(struct mtar_io * io) {
	mtar_io_ios = realloc(mtar_io_ios, (mtar_io_nbIos + 1) * sizeof(struct mtar_io *));
	mtar_io_ios[mtar_io_nbIos] = io;
	mtar_io_nbIos++;

	mtar_loader_register_ok();
}

void mtar_io_show_description() {
	mtar_loader_loadAll("io");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\nList of available backend ios :\n");

	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		mtar_io_ios[i]->show_description();
}

