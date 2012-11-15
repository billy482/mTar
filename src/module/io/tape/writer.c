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
*  Last modified: Thu, 15 Nov 2012 19:30:31 +0100                           *
\***************************************************************************/

// errno
#include <errno.h>
// bool
#include <stdbool.h>
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

#include "tape.h"

struct mtar_io_tape_writer {
	int fd;
	off_t position;
	ssize_t total_free_space;
	int last_errno;
	bool eod_reached;

	char * buffer;
	ssize_t buffer_size;
	ssize_t buffer_used;
};

static ssize_t mtar_io_tape_writer_available_space(struct mtar_io_writer * io);
static ssize_t mtar_io_tape_writer_block_size(struct mtar_io_writer * io);
static int mtar_io_tape_writer_close(struct mtar_io_writer * io);
static int mtar_io_tape_writer_flush(struct mtar_io_writer * io);
static void mtar_io_tape_writer_free(struct mtar_io_writer * io);
static int mtar_io_tape_writer_last_errno(struct mtar_io_writer * io);
static ssize_t mtar_io_tape_writer_next_prefered_size(struct mtar_io_writer * io);
static off_t mtar_io_tape_writer_position(struct mtar_io_writer * io);
static struct mtar_io_reader * mtar_io_tape_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option);
static ssize_t mtar_io_tape_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length);

static struct mtar_io_writer_ops mtar_io_tape_writer_ops = {
	.available_space    = mtar_io_tape_writer_available_space,
	.block_size         = mtar_io_tape_writer_block_size,
	.close              = mtar_io_tape_writer_close,
	.flush              = mtar_io_tape_writer_flush,
	.free               = mtar_io_tape_writer_free,
	.last_errno         = mtar_io_tape_writer_last_errno,
	.next_prefered_size = mtar_io_tape_writer_next_prefered_size,
	.position           = mtar_io_tape_writer_position,
	.reopen_for_reading = mtar_io_tape_writer_reopen_for_reading,
	.write              = mtar_io_tape_writer_write,
};


static ssize_t mtar_io_tape_writer_available_space(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;

	if (self->eod_reached)
		return self->buffer_used > 0 ? self->buffer_size - self->buffer_used : 0;

	return self->total_free_space ? self->total_free_space - self->position : -1 ;
}

static ssize_t mtar_io_tape_writer_block_size(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;
	return self->buffer_size;
}

static int mtar_io_tape_writer_close(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;

	if (self->buffer_used > 0) {
		bzero(self->buffer + self->buffer_used, self->buffer_size - self->buffer_used);
		ssize_t nb_write = write(self->fd, self->buffer, self->buffer_size);

		if (nb_write < 0) {
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

static int mtar_io_tape_writer_flush(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;
	self->last_errno = 0;
	return 0;
}

static void mtar_io_tape_writer_free(struct mtar_io_writer * io) {
	mtar_io_tape_writer_close(io);

	struct mtar_io_tape_writer * self = io->data;
	free(self->buffer);

	free(io->data);
	free(io);
}

static int mtar_io_tape_writer_last_errno(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;
	return self->last_errno;
}

static ssize_t mtar_io_tape_writer_next_prefered_size(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;
	return self->buffer_size - self->buffer_used;
}

static off_t mtar_io_tape_writer_position(struct mtar_io_writer * io) {
	struct mtar_io_tape_writer * self = io->data;
	return self->position;
}

static struct mtar_io_reader * mtar_io_tape_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_io_tape_writer * self = io->data;

	if (self->fd < 0)
		return 0;

	if (self->buffer_used > 0) {
		bzero(self->buffer + self->buffer_used, self->buffer_size - self->buffer_used);
		ssize_t nb_write = write(self->fd, self->buffer, self->buffer_size);

		if (nb_write < 0) {
			self->last_errno = errno;
			return 0;
		}

		self->buffer_used = 0;
	}

	struct mtget status;
	int failed = ioctl(self->fd, MTIOCGET, &status);
	if (failed)
		return 0;

	static const struct mtop eof = { MTWEOF, 1 };
	failed = ioctl(self->fd, MTIOCTOP, &eof);
	if (failed)
		return 0;

	if (status.mt_fileno > 0) {
		static const struct mtop bsfm = { MTBSFM, 2 };
		failed = ioctl(self->fd, MTIOCTOP, &bsfm);
	} else {
		static const struct mtop rewind = { MTREW, 1 };
		failed = ioctl(self->fd, MTIOCTOP, &rewind);
	}

	if (failed)
		return 0;

	struct mtar_io_reader * in = mtar_io_tape_new_reader(self->fd, option);
	if (in != NULL)
		self->fd = -1;

	return in;
}

static ssize_t mtar_io_tape_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length) {
	struct mtar_io_tape_writer * self = io->data;

	if (length < 1)
		return 0;

	self->last_errno = 0;

	ssize_t buffer_available = self->buffer_size - self->buffer_used;
	if (buffer_available > length) {
		memcpy(self->buffer + self->buffer_used, data, length);

		self->buffer_used += length;
		self->position += length;
		return length;
	}

	memcpy(self->buffer + self->buffer_used, data, buffer_available);
	ssize_t nb_write = write(self->fd, self->buffer, self->buffer_size);

	if (nb_write < 0) {
		switch (errno) {
			case ENOSPC:
				self->eod_reached = true;
				nb_write = write(self->fd, self->buffer, self->buffer_size);

				if (nb_write == self->buffer_size)
					break;

			default:
				self->last_errno = errno;
				return nb_write;
		}
	}

	ssize_t nb_total_write = buffer_available;
	self->buffer_used = 0;
	self->position += buffer_available;

	const char * c_buffer = data;
	while (length - nb_total_write >= self->buffer_size) {
		nb_write = write(self->fd, c_buffer + nb_total_write, self->buffer_size);

		if (nb_write < 0) {
			switch (errno) {
				case ENOSPC:
					self->eod_reached = true;
					nb_write = write(self->fd, self->buffer, self->buffer_size);

					if (nb_write == self->buffer_size)
						break;

				default:
					self->last_errno = errno;
					return nb_write;
			}
		}

		nb_total_write += nb_write;
		self->position += nb_write;
	}

	if (length == nb_total_write)
		return length;

	self->buffer_used = length - nb_total_write;
	self->position += self->buffer_used;
	memcpy(self->buffer, c_buffer + nb_total_write, self->buffer_used);

	return length;
}

struct mtar_io_writer * mtar_io_tape_new_writer(int fd, const struct mtar_option * option) {
	struct mtar_io_tape_writer * data = malloc(sizeof(struct mtar_io_tape_writer));
	data->fd = fd;
	data->position = 0;
	data->total_free_space = 0;
	data->last_errno = 0;
	data->eod_reached = false;

	data->buffer = malloc(option->block_factor << 9);
	data->buffer_size = option->block_factor << 9;
	data->buffer_used = 0;

	if (option->tape_length > 0) {
		data->total_free_space = option->tape_length << 10;
	} else {
		off_t tape_position;
		ssize_t tape_free, tape_size;

		int failed = mtar_io_tape_scsi_read_position(fd, &tape_position);
		if (!failed)
			failed = mtar_io_tape_scsi_read_capacity(fd, &tape_free, &tape_size);

		if (!failed && tape_free != tape_size && tape_position > 0)
			data->total_free_space = tape_free;
		else if (!failed)
			data->total_free_space = tape_size - tape_position * data->buffer_size;
		else
			data->total_free_space = data->buffer_size;
	}

	struct mtar_io_writer * io = malloc(sizeof(struct mtar_io_writer));
	io->ops = &mtar_io_tape_writer_ops;
	io->data = data;

	return io;
}

