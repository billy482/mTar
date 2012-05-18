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
*  Last modified: Fri, 18 May 2012 22:07:57 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc
#include <stdlib.h>
// lseek, read
#include <unistd.h>

#include "file.h"

static ssize_t mtar_io_file_in_block_size(struct mtar_io_in * io);
static int mtar_io_file_in_close(struct mtar_io_in * io);
static off_t mtar_io_file_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_io_file_in_free(struct mtar_io_in * io);
static int mtar_io_file_in_last_errno(struct mtar_io_in * io);
static off_t mtar_io_file_in_position(struct mtar_io_in * io);
static ssize_t mtar_io_file_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_io_file_in_ops = {
	.block_size = mtar_io_file_in_block_size,
	.close      = mtar_io_file_in_close,
	.forward    = mtar_io_file_in_forward,
	.free       = mtar_io_file_in_free,
	.last_errno = mtar_io_file_in_last_errno,
	.pos        = mtar_io_file_in_position,
	.read       = mtar_io_file_in_read,
};


ssize_t mtar_io_file_in_block_size(struct mtar_io_in * io) {
	return mtar_io_file_common_block_size(io->data);
}

int mtar_io_file_in_close(struct mtar_io_in * io) {
	return mtar_io_file_common_close(io->data);
}

off_t mtar_io_file_in_forward(struct mtar_io_in * io, off_t offset) {
	struct mtar_io_file * self = io->data;

	off_t ok = lseek(self->fd, offset, SEEK_CUR);
	if (ok == (off_t) -1)
		self->last_errno = errno;

	if (ok >= 0)
		self->position = ok;

	return ok;
}

void mtar_io_file_in_free(struct mtar_io_in * io) {
	mtar_io_file_common_close(io->data);

	free(io->data);
	free(io);
}

int mtar_io_file_in_last_errno(struct mtar_io_in * io) {
	struct mtar_io_file * self = io->data;
	return self->last_errno;
}

off_t mtar_io_file_in_position(struct mtar_io_in * io) {
	struct mtar_io_file * self = io->data;
	return self->position;
}

ssize_t mtar_io_file_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nbRead = read(self->fd, data, length);

	if (nbRead > 0)
		self->position += nbRead;
	else if (nbRead < 0)
		self->last_errno = errno;

	return nbRead;
}

struct mtar_io_in * mtar_io_file_new_in(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_file * self = malloc(sizeof(struct mtar_io_file));
	self->fd = fd;
	self->position = 0;
	self->last_errno = 0;
	self->volume_size = 0;

	struct mtar_io_in * io = malloc(sizeof(struct mtar_io_in));
	io->ops = &mtar_io_file_in_ops;
	io->data = self;

	return io;
}

