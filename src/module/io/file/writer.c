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
*  Last modified: Thu, 15 Nov 2012 19:28:20 +0100                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// fstatfs
#include <sys/statfs.h>
// lseek
#include <sys/types.h>
// fdatasync, lseek, write
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "file.h"

static ssize_t mtar_io_file_writer_available_space(struct mtar_io_writer * io);
static ssize_t mtar_io_file_writer_block_size(struct mtar_io_writer * io);
static int mtar_io_file_writer_close(struct mtar_io_writer * io);
static int mtar_io_file_writer_flush(struct mtar_io_writer * io);
static void mtar_io_file_writer_free(struct mtar_io_writer * io);
static int mtar_io_file_writer_last_errno(struct mtar_io_writer * io);
static ssize_t mtar_io_file_writer_next_prefered_size(struct mtar_io_writer * io);
static off_t mtar_io_file_writer_position(struct mtar_io_writer * io);
static struct mtar_io_reader * mtar_io_file_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option);
static ssize_t mtar_io_file_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length);

static struct mtar_io_writer_ops mtar_io_file_writer_ops = {
	.available_space    = mtar_io_file_writer_available_space,
	.block_size         = mtar_io_file_writer_block_size,
	.close              = mtar_io_file_writer_close,
	.flush              = mtar_io_file_writer_flush,
	.free               = mtar_io_file_writer_free,
	.last_errno         = mtar_io_file_writer_last_errno,
	.next_prefered_size = mtar_io_file_writer_next_prefered_size,
	.position           = mtar_io_file_writer_position,
	.reopen_for_reading = mtar_io_file_writer_reopen_for_reading,
	.write              = mtar_io_file_writer_write,
};


static ssize_t mtar_io_file_writer_available_space(struct mtar_io_writer * io) {
	struct mtar_io_file * self = io->data;

	if (self->volume_size > 0)
		return self->volume_size - self->position;

	struct statfs fs;
	int failed = fstatfs(self->fd, &fs);

	if (failed)
		return -1;

	return fs.f_bsize * fs.f_bavail;
}

static ssize_t mtar_io_file_writer_block_size(struct mtar_io_writer * io) {
	return mtar_io_file_common_block_size(io->data);
}

static int mtar_io_file_writer_close(struct mtar_io_writer * io) {
	return mtar_io_file_common_close(io->data);
}

static int mtar_io_file_writer_flush(struct mtar_io_writer * io) {
	struct mtar_io_file * self = io->data;

	int failed = fdatasync(self->fd);
	if (failed)
		self->last_errno = errno;

	return failed;
}

static void mtar_io_file_writer_free(struct mtar_io_writer * io) {
	mtar_io_file_common_close(io->data);

	free(io->data);
	free(io);
}

static int mtar_io_file_writer_last_errno(struct mtar_io_writer * io) {
	struct mtar_io_file * self = io->data;
	return self->last_errno;
}

static ssize_t mtar_io_file_writer_next_prefered_size(struct mtar_io_writer * io) {
	struct mtar_io_file * self = io->data;
	ssize_t block_size = mtar_io_file_common_block_size(self);

	if (self->volume_size > 0 && self->position + block_size > self->volume_size)
		return self->volume_size == self->position ? 0 : self->volume_size - self->position;

	ssize_t next_size = self->position % block_size;
	return next_size == 0 ? block_size : next_size;
}

static off_t mtar_io_file_writer_position(struct mtar_io_writer * io) {
	struct mtar_io_file * self = io->data;
	return self->position;
}

static struct mtar_io_reader * mtar_io_file_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return NULL;

	off_t position = lseek(self->fd, 0, SEEK_SET);
	if (position == (off_t) -1) {
		// lseek error
		self->last_errno = errno;
		return 0;
	}

	struct mtar_io_reader * in = mtar_io_file_new_reader(self->fd, option);
	if (in != NULL)
		self->fd = -1;

	return in;
}

static ssize_t mtar_io_file_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return 0;

	self->last_errno = 0;

	if (length < 1)
		return 0;

	if (self->volume_size > 0 && self->position + length > self->volume_size) {
		if (self->volume_size == self->position) {
			self->last_errno = ENOSPC;
			return -1;
		}
		length = self->volume_size - self->position;
	}

	ssize_t nb_write = write(self->fd, data, length);

	if (nb_write > 0)
		self->position += nb_write;
	else if (nb_write < 0)
		self->last_errno = errno;

	return nb_write;
}

struct mtar_io_writer * mtar_io_file_new_writer(int fd, const struct mtar_option * option) {
	struct mtar_io_file * self = malloc(sizeof(struct mtar_io_file));
	self->fd = fd;
	self->position = 0;
	self->last_errno = 0;
	self->block_size = 0;
	self->volume_size = 0;

	if (option->multi_volume) {
		if (option->tape_length > 0)
			self->volume_size = option->tape_length << 10;
		else
			mtar_verbose_printf("Warning: discard parameter '-M' because parameter '-L' is not specified\n");
	}

	struct mtar_io_writer * io = malloc(sizeof(struct mtar_io_writer));
	io->ops = &mtar_io_file_writer_ops;
	io->data = self;

	return io;
}

