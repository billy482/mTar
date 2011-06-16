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
*  Last modified: Mon, 13 Jun 2011 11:43:32 +0200                       *
\***********************************************************************/

// free
#include <stdlib.h>
// read
#include <unistd.h>

#include "common.h"

static int mtar_io_pipe_out_close(struct mtar_io_out * io);
static int mtar_io_pipe_out_flush(struct mtar_io_out * io);
static void mtar_io_pipe_out_free(struct mtar_io_out * io);
static off_t mtar_io_pipe_out_pos(struct mtar_io_out * io);
static ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_io_pipe_out_ops = {
	.close = mtar_io_pipe_out_close,
	.flush = mtar_io_pipe_out_flush,
	.free  = mtar_io_pipe_out_free,
	.pos   = mtar_io_pipe_out_pos,
	.write = mtar_io_pipe_out_write,
};


int mtar_io_pipe_out_close(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (!failed)
		self->fd = -1;

	return failed;
}

int mtar_io_pipe_out_flush(struct mtar_io_out * io __attribute__((unused))) {
	return 0;
}

void mtar_io_pipe_out_free(struct mtar_io_out * io) {
	mtar_io_pipe_out_close(io);

	free(io->data);
	free(io);
}

off_t mtar_io_pipe_out_pos(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;
	return self->pos;
}

ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_io_pipe * self = io->data;

	ssize_t nbWrite = write(self->fd, data, length);

	if (nbWrite > 0)
		self->pos += nbWrite;

	return nbWrite;
}

struct mtar_io_out * mtar_io_pipe_newOut(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_pipe * data = malloc(sizeof(struct mtar_io_pipe));
	data->fd = fd;
	data->pos = 0;

	struct mtar_io_out * io = malloc(sizeof(struct mtar_io_out));
	io->ops = &mtar_io_pipe_out_ops;
	io->data = data;

	return io;
}
