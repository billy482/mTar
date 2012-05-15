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
*  Last modified: Tue, 15 May 2012 21:35:11 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// lseek
#include <sys/types.h>
// fdatasync, lseek, write
#include <unistd.h>

#include "file.h"

static ssize_t mtar_io_file_out_block_size(struct mtar_io_out * io);
static int mtar_io_file_out_close(struct mtar_io_out * io);
static int mtar_io_file_out_flush(struct mtar_io_out * io);
static void mtar_io_file_out_free(struct mtar_io_out * io);
static int mtar_io_file_out_last_errno(struct mtar_io_out * io);
static off_t mtar_io_file_out_position(struct mtar_io_out * io);
static struct mtar_io_in * mtar_io_file_out_reopen_for_reading(struct mtar_io_out * io, const struct mtar_option * option);
static ssize_t mtar_io_file_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_io_file_out_ops = {
	.block_size         = mtar_io_file_out_block_size,
	.close              = mtar_io_file_out_close,
	.flush              = mtar_io_file_out_flush,
	.free               = mtar_io_file_out_free,
	.last_errno         = mtar_io_file_out_last_errno,
	.position           = mtar_io_file_out_position,
	.reopen_for_reading = mtar_io_file_out_reopen_for_reading,
	.write              = mtar_io_file_out_write,
};


ssize_t mtar_io_file_out_block_size(struct mtar_io_out * io) {
	return mtar_io_file_common_block_size(io->data);
}

int mtar_io_file_out_close(struct mtar_io_out * io) {
	return mtar_io_file_common_close(io->data);
}

int mtar_io_file_out_flush(struct mtar_io_out * io) {
	struct mtar_io_file * self = io->data;

	int failed = fdatasync(self->fd);
	if (failed)
		self->last_errno = errno;

	return failed;
}

void mtar_io_file_out_free(struct mtar_io_out * io) {
	mtar_io_file_common_close(io->data);

	free(io->data);
	free(io);
}

int mtar_io_file_out_last_errno(struct mtar_io_out * io) {
	struct mtar_io_file * self = io->data;
	return self->last_errno;
}

off_t mtar_io_file_out_position(struct mtar_io_out * io) {
	struct mtar_io_file * self = io->data;
	return self->pos;
}

struct mtar_io_in * mtar_io_file_out_reopen_for_reading(struct mtar_io_out * io, const struct mtar_option * option) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return 0;

	off_t pos = lseek(self->fd, 0, SEEK_SET);
	if (pos > 0) {
		// lseek error
		self->last_errno = errno;
		return 0;
	}

	struct mtar_io_in * in = mtar_io_file_new_in(self->fd, 0, option);
	if (in)
		self->fd = -1;

	return in;
}

ssize_t mtar_io_file_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nbWrite = write(self->fd, data, length);

	if (nbWrite > 0)
		self->pos += nbWrite;
	else if (nbWrite < 0)
		self->last_errno = errno;

	return nbWrite;
}

struct mtar_io_out * mtar_io_file_new_out(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_file * data = malloc(sizeof(struct mtar_io_file));
	data->fd = fd;
	data->pos = 0;
	data->last_errno = 0;

	struct mtar_io_out * io = malloc(sizeof(struct mtar_io_out));
	io->ops = &mtar_io_file_out_ops;
	io->data = data;

	return io;
}

