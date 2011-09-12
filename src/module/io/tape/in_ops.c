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
*  Last modified: Mon, 12 Sep 2011 17:37:17 +0200                       *
\***********************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// memcpy
#include <string.h>
// close, read
#include <unistd.h>

#include <mtar/option.h>

#include "common.h"

struct mtar_io_tape_in {
	int fd;
	off_t pos;
	int last_errno;

	char * buffer;
	unsigned int buffer_size;
	unsigned int buffer_used;
	char * buffer_pos;
};

static ssize_t mtar_io_tape_in_block_size(struct mtar_io_in * io);
static int mtar_io_tape_in_close(struct mtar_io_in * io);
static off_t mtar_io_tape_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_io_tape_in_free(struct mtar_io_in * io);
static int mtar_io_tape_in_last_errno(struct mtar_io_in * io);
static off_t mtar_io_tape_in_pos(struct mtar_io_in * io);
static ssize_t mtar_io_tape_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_io_tape_in_ops = {
	.block_size = mtar_io_tape_in_block_size,
	.close      = mtar_io_tape_in_close,
	.forward    = mtar_io_tape_in_forward,
	.free       = mtar_io_tape_in_free,
	.last_errno = mtar_io_tape_in_last_errno,
	.pos        = mtar_io_tape_in_pos,
	.read       = mtar_io_tape_in_read,
};


ssize_t mtar_io_tape_in_block_size(struct mtar_io_in * io) {
	struct mtar_io_tape_in * self = io->data;
	return self->buffer_size;
}

int mtar_io_tape_in_close(struct mtar_io_in * io) {
	struct mtar_io_tape_in * self = io->data;

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (failed)
		self->last_errno = errno;
	else
		self->fd = -1;

	return failed;
}

off_t mtar_io_tape_in_forward(struct mtar_io_in * io, off_t offset) {
	char buffer[4096];

	while (offset > 0) {
		ssize_t read = offset;
		if (read > 4096)
			read = 4096;

		ssize_t nbRead = mtar_io_tape_in_read(io, buffer, read);

		if (nbRead > 0)
			offset -= nbRead;
		else if (nbRead == 0)
			break;
		else if (nbRead < 0)
			return -1;
	}

	struct mtar_io_tape_in * self = io->data;
	return self->pos;
}

void mtar_io_tape_in_free(struct mtar_io_in * io) {
	mtar_io_tape_in_close(io);

	struct mtar_io_tape_in * self = io->data;

	free(self->buffer);
	free(io->data);
	free(io);
}

int mtar_io_tape_in_last_errno(struct mtar_io_in * io) {
	struct mtar_io_tape_in * self = io->data;
	return self->last_errno;
}

off_t mtar_io_tape_in_pos(struct mtar_io_in * io) {
	struct mtar_io_tape_in * self = io->data;
	return self->pos;
}

ssize_t mtar_io_tape_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_io_tape_in * self = io->data;

	ssize_t available = self->buffer_used - (self->buffer_pos - self->buffer);
	if (available > 0) {
		if (available == length) {
			memcpy(data, self->buffer_pos, length);

			self->pos += length;
			self->buffer_used = 0;
			self->buffer_pos = self->buffer;
			return length;
		} else if (available > length) {
			memcpy(data, self->buffer_pos, length);

			self->pos += length;
			self->buffer_pos += length;
			return length;
		} else {
			ssize_t need_read = length - available;
			if (need_read % self->buffer_size)
				need_read += (self->buffer_size - available) % self->buffer_size;

			char * buffer = malloc(need_read);
			ssize_t nbRead = read(self->fd, buffer, need_read);

			if (nbRead >= 0) {
				memcpy(data, self->buffer_pos, available);
				self->pos += available;
				self->buffer_used = 0;
				self->buffer_pos = self->buffer;

				if (nbRead > 0) {
					ssize_t copy_size = length - available;
					char * pdata = data;
					if (need_read > copy_size) {
						memcpy(pdata + available, buffer, copy_size);

						memcpy(self->buffer, buffer + copy_size, nbRead - copy_size);
						self->pos += copy_size;
						self->buffer_used = nbRead - copy_size;
					} else
						memcpy(pdata + available, buffer, copy_size);

					free(buffer);

					return length;
				} else {
					free(buffer);
					return available;
				}
			} else {
				free(buffer);
				self->last_errno = errno;
				return nbRead;
			}
		}
	} else if (length % self->buffer_size == 0) {
		ssize_t nbRead = read(self->fd, data, length);

		if (nbRead > 0)
			self->pos += length;
		else if (nbRead < 0)
			self->last_errno = errno;

		return nbRead;
	} else {
		ssize_t need_read = length - available + (self->buffer_size - available) % self->buffer_size;
		char * buffer = malloc(need_read);
		ssize_t nbRead = read(self->fd, buffer, need_read);

		if (nbRead > 0) {
			memcpy(data, buffer, length);
			self->pos += length;

			memcpy(self->buffer, buffer + length, need_read - length);
			self->buffer_used = need_read - length;

			free(buffer);
			return nbRead;
		} else if (nbRead == 0) {
			free(buffer);
			return 0;
		} else {
			free(buffer);
			self->last_errno = errno;
			return nbRead;
		}
	}
}

struct mtar_io_in * mtar_io_tape_new_in(int fd, int flags __attribute__((unused)), const struct mtar_option * option) {
	struct mtar_io_tape_in * data = malloc(sizeof(struct mtar_io_tape_in));
	data->fd = fd;
	data->pos = 0;
	data->last_errno = 0;

	data->buffer = malloc(option->block_factor << 9);
	data->buffer_size = option->block_factor << 9;
	data->buffer_used = 0;
	data->buffer_pos = data->buffer;

	struct mtar_io_in * io = malloc(sizeof(struct mtar_io_in));
	io->ops = &mtar_io_tape_in_ops;
	io->data = data;

	return io;
}

