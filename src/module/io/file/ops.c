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
*  Last modified: Thu, 21 Apr 2011 22:29:17 +0200                       *
\***********************************************************************/

// free
#include <stdlib.h>
// lseek
#include <sys/types.h>
// close, lseek
#include <unistd.h>

#include "common.h"

int mtar_io_file_can_seek(struct mtar_io * io __attribute__((unused))) {
	return 1;
}

int mtar_io_file_close(struct mtar_io * io) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);

	if (!failed)
		self->fd = -1;

	return failed;
}

void mtar_io_file_free(struct mtar_io * io) {
	struct mtar_io_file * self = io->data;

	if (self->fd >= 0)
		mtar_io_file_close(io);

	free(self);
	free(io);
}

off_t mtar_io_file_pos(struct mtar_io * io) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return -1;

	return lseek(self->fd, 0, SEEK_CUR);
}

ssize_t mtar_io_file_read(struct mtar_io * io, void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nbRead = read(self->fd, data, length);

	if (nbRead > 0)
		self->pos += nbRead;

	return nbRead;
}

off_t mtar_io_file_seek(struct mtar_io * io, off_t offset, int whence) {
	struct mtar_io_file * self = io->data;

	if (self->fd < 0)
		return -1;

	return lseek(self->fd, offset, whence);
}

ssize_t mtar_io_file_write(struct mtar_io * io, const void * data, ssize_t length) {
	struct mtar_io_file * self = io->data;

	ssize_t nbWrite = write(self->fd, data, length);

	if (nbWrite > 0)
		self->pos += nbWrite;

	return nbWrite;
}
