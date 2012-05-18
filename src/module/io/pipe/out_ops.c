/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar (modular tar) is free software; you can redistribute it and/or      *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program; if not, write to the Free Software              *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                       *
*  Boston, MA  02110-1301, USA.                                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2012, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Fri, 18 May 2012 22:28:38 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// write
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "pipe.h"

static ssize_t mtar_io_pipe_out_available_space(struct mtar_io_out * io);
static ssize_t mtar_io_pipe_out_block_size(struct mtar_io_out * io);
static int mtar_io_pipe_out_close(struct mtar_io_out * io);
static int mtar_io_pipe_out_flush(struct mtar_io_out * io);
static void mtar_io_pipe_out_free(struct mtar_io_out * io);
static int mtar_io_pipe_out_last_errno(struct mtar_io_out * io);
static off_t mtar_io_pipe_out_position(struct mtar_io_out * io);
static struct mtar_io_in * mtar_io_pipe_out_reopen_for_reading(struct mtar_io_out * io, const struct mtar_option * option);
static ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_io_pipe_out_ops = {
	.available_space    = mtar_io_pipe_out_available_space,
	.block_size         = mtar_io_pipe_out_block_size,
	.close              = mtar_io_pipe_out_close,
	.flush              = mtar_io_pipe_out_flush,
	.free               = mtar_io_pipe_out_free,
	.last_errno         = mtar_io_pipe_out_last_errno,
	.position           = mtar_io_pipe_out_position,
	.reopen_for_reading = mtar_io_pipe_out_reopen_for_reading,
	.write              = mtar_io_pipe_out_write,
};


ssize_t mtar_io_pipe_out_available_space(struct mtar_io_out * io __attribute__((unused))) {
	return -1;
}

ssize_t mtar_io_pipe_out_block_size(struct mtar_io_out * io) {
	return mtar_io_pipe_common_block_size(io->data);
}

int mtar_io_pipe_out_close(struct mtar_io_out * io) {
	return mtar_io_pipe_common_close(io->data);
}

int mtar_io_pipe_out_flush(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;
	self->last_errno = 0;
	return 0;
}

void mtar_io_pipe_out_free(struct mtar_io_out * io) {
	mtar_io_pipe_common_close(io->data);

	free(io->data);
	free(io);
}

int mtar_io_pipe_out_last_errno(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;
	return self->last_errno;
}

off_t mtar_io_pipe_out_position(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;
	return self->position;
}

ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_io_pipe * self = io->data;

	if (length < 1)
		return 0;

	self->last_errno = 0;

	ssize_t nb_write = write(self->fd, data, length);

	if (nb_write > 0)
		self->position += nb_write;
	else if (nb_write < 0)
		self->last_errno = errno;

	return nb_write;
}

struct mtar_io_in * mtar_io_pipe_out_reopen_for_reading(struct mtar_io_out * io __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	return 0;
}

struct mtar_io_out * mtar_io_pipe_new_out(int fd, int flags __attribute__((unused)), const struct mtar_option * option) {
	if (option->multi_volume) {
		mtar_verbose_printf("Error: I'm not able to write a multi volume archive into a pipe\n");
		return 0;
	}

	struct mtar_io_pipe * self = malloc(sizeof(struct mtar_io_pipe));
	self->fd = fd;
	self->position = 0;
	self->last_errno = 0;

	struct mtar_io_out * io = malloc(sizeof(struct mtar_io_out));
	io->ops = &mtar_io_pipe_out_ops;
	io->data = self;

	return io;
}

