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
*  Last modified: Sun, 03 Jul 2011 20:32:58 +0200                       *
\***********************************************************************/

// errno
#include <errno.h>
// free
#include <stdlib.h>
// lseek
#include <sys/types.h>
// lseek, read
#include <unistd.h>

#include "common.h"

static int mtar_io_file_in_close(struct mtar_io_in * io);
static off_t mtar_io_file_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_io_file_in_free(struct mtar_io_in * io);
static int mtar_io_file_last_errno(struct mtar_io_in * io);
static off_t mtar_io_file_in_pos(struct mtar_io_in * io);
static ssize_t mtar_io_file_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_io_file_in_ops = {
	.close      = mtar_io_file_in_close,
	.forward    = mtar_io_file_in_forward,
	.free       = mtar_io_file_in_free,
	.last_errno = mtar_io_file_last_errno,
	.pos        = mtar_io_file_in_pos,
	.read       = mtar_io_file_in_read,
};


int mtar_io_file_in_close(struct mtar_io_in * io) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (failed)
		self->last_errno = errno;
	else
		self->fd = -1;

	return failed;
}

off_t mtar_io_file_in_forward(struct mtar_io_in * io, off_t offset) {
	struct mtar_io_file * self = io->data;

	off_t ok = lseek(self->fd, offset, SEEK_CUR);
	if (ok == (off_t) -1)
		self->last_errno = errno;

	return ok;
}

void mtar_io_file_in_free(struct mtar_io_in * io) {
	mtar_io_file_in_close(io);

	free(io->data);
	free(io);
}

int mtar_io_file_last_errno(struct mtar_io_in * io) {
	struct mtar_io_file * self = io->data;
	return self->last_errno;
}

off_t mtar_io_file_in_pos(struct mtar_io_in * io) {
	struct mtar_io_file * self = io->data;
	return self->pos;
}

ssize_t mtar_io_file_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nbRead = read(self->fd, data, length);

	if (nbRead > 0)
		self->pos += nbRead;
	else if (nbRead < 0)
		self->last_errno = errno;

	return nbRead;
}

struct mtar_io_in * mtar_io_file_new_in(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_file * data = malloc(sizeof(struct mtar_io_file));
	data->fd = fd;
	data->pos = 0;
	data->last_errno = 0;

	struct mtar_io_in * io = malloc(sizeof(struct mtar_io_in));
	io->ops = &mtar_io_file_in_ops;
	io->data = data;
	io->driver = mtar_io_file_get_driver();

	return io;
}

