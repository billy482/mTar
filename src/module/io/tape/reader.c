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
*  Last modified: Sun, 28 Oct 2012 16:20:37 +0100                           *
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
// close, read
#include <unistd.h>

#include <mtar/option.h>

#include "tape.h"

struct mtar_io_tape_reader {
	int fd;
	off_t position;
	int last_errno;

	char * buffer;
	ssize_t block_size;
	char * buffer_pos;
};

static ssize_t mtar_io_tape_reader_block_size(struct mtar_io_reader * io);
static int mtar_io_tape_reader_close(struct mtar_io_reader * io);
static off_t mtar_io_tape_reader_forward(struct mtar_io_reader * io, off_t offset);
static void mtar_io_tape_reader_free(struct mtar_io_reader * io);
static int mtar_io_tape_reader_last_errno(struct mtar_io_reader * io);
static off_t mtar_io_tape_reader_position(struct mtar_io_reader * io);
static ssize_t mtar_io_tape_reader_read(struct mtar_io_reader * io, void * data, ssize_t length);

static struct mtar_io_reader_ops mtar_io_tape_reader_ops = {
	.block_size = mtar_io_tape_reader_block_size,
	.close      = mtar_io_tape_reader_close,
	.forward    = mtar_io_tape_reader_forward,
	.free       = mtar_io_tape_reader_free,
	.last_errno = mtar_io_tape_reader_last_errno,
	.position   = mtar_io_tape_reader_position,
	.read       = mtar_io_tape_reader_read,
};


static ssize_t mtar_io_tape_reader_block_size(struct mtar_io_reader * io) {
	struct mtar_io_tape_reader * self = io->data;
	return self->block_size;
}

static int mtar_io_tape_reader_close(struct mtar_io_reader * io) {
	struct mtar_io_tape_reader * self = io->data;

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (failed)
		self->last_errno = errno;
	else
		self->fd = -1;

	return failed;
}

static off_t mtar_io_tape_reader_forward(struct mtar_io_reader * io, off_t offset) {
	struct mtar_io_tape_reader * self = io->data;

	ssize_t nb_total_read = self->block_size - (self->buffer_pos - self->buffer);
	if (nb_total_read > 0) {
		ssize_t will_move = nb_total_read > offset ? offset : nb_total_read;

		self->buffer_pos += will_move;
		self->position += will_move;

		if (will_move == offset)
			return self->position;
	}

	if (offset - nb_total_read >= self->block_size) {
		long nb_records = (offset - nb_total_read) / self->block_size;

		/**
		 * There is a limitation with scsi command 'space' used by linux driver st.
		 * With this command block_number is specified into 3 bytes so 8388607 is the
		 * maximum that we can forward each time.
		 */
		while (nb_records > 0) {
			int next_forward = nb_records > 8388607 ? 8388607 : nb_records;

			struct mtop forward = { MTFSR, next_forward };
			int failed = ioctl(self->fd, MTIOCTOP, &forward);
			if (failed) {
				self->last_errno = errno;
				return failed;
			}

			nb_records -= next_forward;

			self->position += next_forward * self->block_size;
			nb_total_read += next_forward * self->block_size;
		}
	}

	if (nb_total_read == offset)
		return self->position;

	ssize_t nb_read = read(self->fd, self->buffer, self->block_size);
	if (nb_read < 0) {
		self->last_errno = errno;
		return nb_read;
	} else if (nb_read > 0) {
		ssize_t will_move = offset - nb_total_read;
		self->buffer_pos = self->buffer + will_move;
		self->position += will_move;
	}

	return self->position;
}

static void mtar_io_tape_reader_free(struct mtar_io_reader * io) {
	mtar_io_tape_reader_close(io);

	struct mtar_io_tape_reader * self = io->data;

	free(self->buffer);
	free(io->data);
	free(io);
}

static int mtar_io_tape_reader_last_errno(struct mtar_io_reader * io) {
	struct mtar_io_tape_reader * self = io->data;
	return self->last_errno;
}

static off_t mtar_io_tape_reader_position(struct mtar_io_reader * io) {
	struct mtar_io_tape_reader * self = io->data;
	return self->position;
}

static ssize_t mtar_io_tape_reader_read(struct mtar_io_reader * io, void * data, ssize_t length) {
	struct mtar_io_tape_reader * self = io->data;

	ssize_t nb_total_read = self->block_size - (self->buffer_pos - self->buffer);
	if (nb_total_read > 0) {
		ssize_t will_copy = nb_total_read > length ? length : nb_total_read;
		memcpy(data, self->buffer_pos, will_copy);

		self->buffer_pos += will_copy;
		self->position += will_copy;

		if (will_copy == length)
			return length;
	}

	char * c_data = data;
	while (length - nb_total_read >= self->block_size) {
		ssize_t nb_read = read(self->fd, c_data + nb_total_read, self->block_size);
		if (nb_read < 0) {
			self->last_errno = errno;
			return nb_read;
		} else if (nb_read > 0) {
			nb_total_read += nb_read;
			self->position += nb_read;
		} else {
			return nb_total_read;
		}
	}

	if (nb_total_read == length)
		return length;

	ssize_t nb_read = read(self->fd, self->buffer, self->block_size);
	if (nb_read < 0) {
		self->last_errno = errno;
		return nb_read;
	} else if (nb_read > 0) {
		ssize_t will_copy = length - nb_total_read;
		memcpy(c_data + nb_total_read, self->buffer, will_copy);
		self->buffer_pos = self->buffer + will_copy;
		self->position += will_copy;
		return length;
	} else {
		return nb_total_read;
	}
}

struct mtar_io_reader * mtar_io_tape_new_reader(int fd, const struct mtar_option * option, const struct mtar_hashtable * params __attribute__((unused))) {
	struct mtar_io_tape_reader * data = malloc(sizeof(struct mtar_io_tape_reader));
	data->fd = fd;
	data->position = 0;
	data->last_errno = 0;

	data->buffer = malloc(option->block_factor << 9);
	data->block_size = option->block_factor << 9;
	data->buffer_pos = data->buffer + data->block_size;

	struct mtar_io_reader * io = malloc(sizeof(struct mtar_io_reader));
	io->ops = &mtar_io_tape_reader_ops;
	io->data = data;

	return io;
}

