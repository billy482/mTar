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
*  Last modified: Sun, 11 Nov 2012 20:22:54 +0100                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// fstat
#include <sys/stat.h>
// fstat
#include <sys/types.h>
// fstat, lseek, read
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "file.h"

static ssize_t mtar_io_file_reader_block_size(struct mtar_io_reader * io);
static int mtar_io_file_reader_close(struct mtar_io_reader * io);
static off_t mtar_io_file_reader_forward(struct mtar_io_reader * io, off_t offset);
static void mtar_io_file_reader_free(struct mtar_io_reader * io);
static int mtar_io_file_reader_last_errno(struct mtar_io_reader * io);
static off_t mtar_io_file_reader_position(struct mtar_io_reader * io);
static ssize_t mtar_io_file_reader_read(struct mtar_io_reader * io, void * data, ssize_t length);

static struct mtar_io_reader_ops mtar_io_file_reader_ops = {
	.block_size = mtar_io_file_reader_block_size,
	.close      = mtar_io_file_reader_close,
	.forward    = mtar_io_file_reader_forward,
	.free       = mtar_io_file_reader_free,
	.last_errno = mtar_io_file_reader_last_errno,
	.position   = mtar_io_file_reader_position,
	.read       = mtar_io_file_reader_read,
};


static ssize_t mtar_io_file_reader_block_size(struct mtar_io_reader * io) {
	return mtar_io_file_common_block_size(io->data);
}

static int mtar_io_file_reader_close(struct mtar_io_reader * io) {
	return mtar_io_file_common_close(io->data);
}

static off_t mtar_io_file_reader_forward(struct mtar_io_reader * io, off_t offset) {
	struct mtar_io_file * self = io->data;

	off_t new_position;
	if (self->volume_size > 0 && self->volume_size < self->position + offset)
		new_position = lseek(self->fd, 0, SEEK_END);
	else
		new_position = lseek(self->fd, offset, SEEK_CUR);

	if (new_position == (off_t) -1)
		self->last_errno = errno;
	else if (new_position >= 0)
		self->position = new_position;

	return new_position;
}

static void mtar_io_file_reader_free(struct mtar_io_reader * io) {
	mtar_io_file_common_close(io->data);

	free(io->data);
	free(io);
}

static int mtar_io_file_reader_last_errno(struct mtar_io_reader * io) {
	struct mtar_io_file * self = io->data;
	return self->last_errno;
}

static off_t mtar_io_file_reader_position(struct mtar_io_reader * io) {
	struct mtar_io_file * self = io->data;
	return self->position;
}

static ssize_t mtar_io_file_reader_read(struct mtar_io_reader * io, void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nb_read = read(self->fd, data, length);

	if (nb_read > 0)
		self->position += nb_read;
	else if (nb_read < 0)
		self->last_errno = errno;

	return nb_read;
}

struct mtar_io_reader * mtar_io_file_new_reader(int fd, const struct mtar_option * option, const struct mtar_hashtable * params __attribute__((unused))) {
	struct mtar_io_file * self = malloc(sizeof(struct mtar_io_file));
	self->fd = fd;
	self->position = 0;
	self->last_errno = 0;
	self->block_size = 0;
	self->volume_size = 0;

	if (option->multi_volume) {
		struct stat st;

		if (!fstat(fd, &st))
			self->volume_size = st.st_size;
	}

	struct mtar_io_reader * io = malloc(sizeof(struct mtar_io_reader));
	io->ops = &mtar_io_file_reader_ops;
	io->data = self;

	return io;
}

