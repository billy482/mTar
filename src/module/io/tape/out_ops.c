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
*  Last modified: Mon, 07 May 2012 23:43:12 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// memcpy
#include <string.h>
// ioctl
#include <sys/ioctl.h>
// MT*
#include <sys/mtio.h>
// bzero, write
#include <unistd.h>

#include <mtar/option.h>

#include "common.h"

struct mtar_io_tape_out {
	int fd;
	off_t pos;
	int last_errno;

	char * buffer;
	unsigned int buffer_size;
	unsigned int buffer_used;
};

static ssize_t mtar_io_tape_out_block_size(struct mtar_io_out * io);
static int mtar_io_tape_out_close(struct mtar_io_out * io);
static int mtar_io_tape_out_flush(struct mtar_io_out * io);
static void mtar_io_tape_out_free(struct mtar_io_out * io);
static int mtar_io_tape_out_last_errno(struct mtar_io_out * io);
static off_t mtar_io_tape_out_pos(struct mtar_io_out * io);
static struct mtar_io_in * mtar_io_tape_out_reopen_for_reading(struct mtar_io_out * io, const struct mtar_option * option);
static ssize_t mtar_io_tape_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_io_tape_out_ops = {
	.block_size         = mtar_io_tape_out_block_size,
	.close              = mtar_io_tape_out_close,
	.flush              = mtar_io_tape_out_flush,
	.free               = mtar_io_tape_out_free,
	.last_errno         = mtar_io_tape_out_last_errno,
	.pos                = mtar_io_tape_out_pos,
	.reopen_for_reading = mtar_io_tape_out_reopen_for_reading,
	.write              = mtar_io_tape_out_write,
};


ssize_t mtar_io_tape_out_block_size(struct mtar_io_out * io) {
	struct mtar_io_tape_out * self = io->data;
	return self->buffer_size;
}

int mtar_io_tape_out_close(struct mtar_io_out * io) {
	struct mtar_io_tape_out * self = io->data;

	if (self->buffer_used > 0) {
		bzero(self->buffer + self->buffer_used, self->buffer_size - self->buffer_used);
		ssize_t nbWrite = write(self->fd, self->buffer, self->buffer_size);

		if (nbWrite < 0) {
			self->last_errno = errno;
			return -1;
		}
	}

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (failed)
		self->last_errno = 0;
	else
		self->fd = -1;

	return failed;
}

int mtar_io_tape_out_flush(struct mtar_io_out * io __attribute__((unused))) {
	return 0;
}

void mtar_io_tape_out_free(struct mtar_io_out * io) {
	mtar_io_tape_out_close(io);

	struct mtar_io_tape_out * self = io->data;
	free(self->buffer);

	free(io->data);
	free(io);
}

int mtar_io_tape_out_last_errno(struct mtar_io_out * io) {
	struct mtar_io_tape_out * self = io->data;
	return self->last_errno;
}

off_t mtar_io_tape_out_pos(struct mtar_io_out * io) {
	struct mtar_io_tape_out * self = io->data;
	return self->pos;
}

ssize_t mtar_io_tape_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_io_tape_out * self = io->data;

	if (self->buffer_used + length < self->buffer_size) {
		memcpy(self->buffer + self->buffer_used, data, length);
		self->buffer_used += length;
		return length;
	} else if (self->buffer_used + length < 2 * self->buffer_size) {
		ssize_t copy_size = self->buffer_size - self->buffer_used;
		memcpy(self->buffer + self->buffer_used, data, copy_size);
		ssize_t nbWrite = write(self->fd, self->buffer, self->buffer_size);

		if (nbWrite == self->buffer_size) {
			const char * pdata = data;
			memcpy(self->buffer, pdata + copy_size, length - copy_size);
			self->buffer_used = length - copy_size;
			return length;
		} else if (nbWrite < 0) {
			self->last_errno = errno;
			return nbWrite;
		}
	} else {
		ssize_t copy_size = (self->buffer_used + length) - (self->buffer_used + length) % self->buffer_size;
		char * buffer = malloc(copy_size);

		memcpy(buffer, self->buffer, self->buffer_used);
		memcpy(buffer + self->buffer_used, data, copy_size - self->buffer_used);

		ssize_t nbWrite = write(self->fd, buffer, copy_size);

		free(buffer);

		if (nbWrite == copy_size) {
			const char * pdata = data;
			memcpy(self->buffer, pdata + (copy_size - self->buffer_used), self->buffer_used + length - copy_size);
			self->buffer_used = self->buffer_used + length - copy_size;
			return length;
		} else if (nbWrite < 0) {
			self->last_errno = errno;
			return nbWrite;
		}
	}

	return -2;
}

struct mtar_io_in * mtar_io_tape_out_reopen_for_reading(struct mtar_io_out * io, const struct mtar_option * option) {
	struct mtar_io_tape_out * self = io->data;

	if (self->fd < 0)
		return 0;

	if (self->buffer_used > 0) {
		bzero(self->buffer + self->buffer_used, self->buffer_size - self->buffer_used);
		ssize_t nbWrite = write(self->fd, self->buffer, self->buffer_size);

		if (nbWrite < 0) {
			self->last_errno = errno;
			return 0;
		}

		self->buffer_used = 0;
	}

	static const struct mtop eof = { MTWEOF, 1 };
	int failed = ioctl(self->fd, MTIOCTOP, &eof);
	if (failed)
		return 0;

	static const struct mtop bsf = { MTBSF, 1 };
	failed = ioctl(self->fd, MTIOCTOP, &bsf);
	if (failed)
		return 0;

	static const struct mtop fsf = { MTFSF, 1 };
	failed = ioctl(self->fd, MTIOCTOP, &fsf);
	if (failed)
		return 0;

	struct mtar_io_in * in = mtar_io_tape_new_in(self->fd, 0, option);
	if (in)
		self->fd = -1;

	return in;
}

struct mtar_io_out * mtar_io_tape_new_out(int fd, int flags __attribute__((unused)), const struct mtar_option * option) {
	struct mtar_io_tape_out * data = malloc(sizeof(struct mtar_io_tape_out));
	data->fd = fd;
	data->pos = 0;
	data->last_errno = 0;

	data->buffer = malloc(option->block_factor << 9);
	data->buffer_size = option->block_factor << 9;
	data->buffer_used = 0;

	struct mtar_io_out * io = malloc(sizeof(struct mtar_io_out));
	io->ops = &mtar_io_tape_out_ops;
	io->data = data;

	return io;
}

