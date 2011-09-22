/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Thu, 22 Sep 2011 10:21:37 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// write
#include <unistd.h>

#include "common.h"

static ssize_t mtar_io_pipe_out_block_size(struct mtar_io_out * io);
static int mtar_io_pipe_out_close(struct mtar_io_out * io);
static int mtar_io_pipe_out_flush(struct mtar_io_out * io);
static void mtar_io_pipe_out_free(struct mtar_io_out * io);
static int mtar_io_pipe_out_last_errno(struct mtar_io_out * io);
static off_t mtar_io_pipe_out_pos(struct mtar_io_out * io);
static struct mtar_io_in * mtar_io_pipe_out_reopenForReading(struct mtar_io_out * io, const struct mtar_option * option);
static ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_io_pipe_out_ops = {
	.block_size       = mtar_io_pipe_out_block_size,
	.close            = mtar_io_pipe_out_close,
	.flush            = mtar_io_pipe_out_flush,
	.free             = mtar_io_pipe_out_free,
	.last_errno       = mtar_io_pipe_out_last_errno,
	.pos              = mtar_io_pipe_out_pos,
	.reopenForReading = mtar_io_pipe_out_reopenForReading,
	.write            = mtar_io_pipe_out_write,
};


ssize_t mtar_io_pipe_out_block_size(struct mtar_io_out * io) {
	return mtar_io_pipe_common_block_size(io->data);
}

int mtar_io_pipe_out_close(struct mtar_io_out * io) {
	return mtar_io_pipe_common_close(io->data);
}

int mtar_io_pipe_out_flush(struct mtar_io_out * io __attribute__((unused))) {
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

off_t mtar_io_pipe_out_pos(struct mtar_io_out * io) {
	struct mtar_io_pipe * self = io->data;
	return self->pos;
}

ssize_t mtar_io_pipe_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_io_pipe * self = io->data;

	ssize_t nbWrite = write(self->fd, data, length);

	if (nbWrite > 0)
		self->pos += nbWrite;
	else if (nbWrite < 0)
		self->last_errno = errno;

	return nbWrite;
}

struct mtar_io_in * mtar_io_pipe_out_reopenForReading(struct mtar_io_out * io __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	return 0;
}

struct mtar_io_out * mtar_io_pipe_new_out(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_pipe * data = malloc(sizeof(struct mtar_io_pipe));
	data->fd = fd;
	data->pos = 0;
	data->last_errno = 0;

	struct mtar_io_out * io = malloc(sizeof(struct mtar_io_out));
	io->ops = &mtar_io_pipe_out_ops;
	io->data = data;

	return io;
}

