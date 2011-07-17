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
*  Last modified: Sun, 17 Jul 2011 20:32:13 +0200                       *
\***********************************************************************/

// free, malloc
#include <malloc.h>
// strchr
#include <string.h>
// inflate, inflateEnd, inflateInit2
#include <zlib.h>

#include "common.h"

struct mtar_filter_gzip_in {
	z_stream gz_stream;
	struct mtar_io_in * io;

	char bufferIn[1024];
};

static int mtar_filter_gzip_in_close(struct mtar_io_in * io);
static off_t mtar_filter_gzip_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_filter_gzip_in_free(struct mtar_io_in * io);
static int mtar_filter_gzip_in_last_errno(struct mtar_io_in * io);
static off_t mtar_filter_gzip_in_pos(struct mtar_io_in * io);
static ssize_t mtar_filter_gzip_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_filter_gzip_in_ops = {
	.close      = mtar_filter_gzip_in_close,
	.forward    = mtar_filter_gzip_in_forward,
	.free       = mtar_filter_gzip_in_free,
	.last_errno = mtar_filter_gzip_in_last_errno,
	.pos        = mtar_filter_gzip_in_pos,
	.read       = mtar_filter_gzip_in_read,
};


int mtar_filter_gzip_in_close(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	inflateEnd(&self->gz_stream);
	return self->io->ops->close(self->io);
}

off_t mtar_filter_gzip_in_forward(struct mtar_io_in * io __attribute__((unused)), off_t offset __attribute__((unused))) {
	return -1;
}

void mtar_filter_gzip_in_free(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_gzip_in_last_errno(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_gzip_in_pos(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	return self->io->ops->pos(self->io);
}

ssize_t mtar_filter_gzip_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_filter_gzip_in * self = io->data;

	self->gz_stream.next_out = data;
	self->gz_stream.avail_out = length;
	self->gz_stream.total_out = 0;

	while (self->gz_stream.avail_out > 0) {
		if (self->gz_stream.avail_in > 0) {
			int err = inflate(&self->gz_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END || self->gz_stream.avail_out == 0)
				return self->gz_stream.total_out;
		}

		int nbRead = self->io->ops->read(self->io, self->bufferIn + self->gz_stream.avail_in, 1024 - self->gz_stream.avail_in);
		if (nbRead > 0) {
			self->gz_stream.next_in = (unsigned char *) self->bufferIn;
			self->gz_stream.avail_in = nbRead;
			self->gz_stream.total_in = 0;
		} else if (nbRead == 0) {
			return self->gz_stream.total_out;
		} else {
			return nbRead;
		}
	}

	return self->gz_stream.total_out;
}

struct mtar_io_in * mtar_filter_gzip_new_in(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_gzip_in * self = malloc(sizeof(struct mtar_filter_gzip_in));
	self->io = io;

	int nbRead = io->ops->read(io, self->bufferIn, 1024);

	if (self->bufferIn[0] != 0x1f || ((unsigned char) self->bufferIn[1]) != 0x8b) {
		free(self);
		return 0;
	}

	// skip gzip header
	unsigned char flag = self->bufferIn[3];
	char * ptr = self->bufferIn + 10;
	if (flag & 0x4) {
		unsigned short length = ptr[0] << 8 | ptr[1];
		ptr += length + 2;
	}
	if (flag & 0x8) {
		char * end = strchr(ptr, 0);
		if (end)
			ptr = end + 1;
	}
	if (flag & 0x10) {
		char * end = strchr(ptr, 0);
		if (end)
			ptr = end + 1;
	}
	if (flag & 0x2)
		ptr += 2;

	// init data
	self->gz_stream.zalloc = 0;
	self->gz_stream.zfree = 0;
	self->gz_stream.opaque = 0;

	self->gz_stream.next_in = (unsigned char *) ptr;
	self->gz_stream.avail_in = nbRead - (ptr - self->bufferIn);

	int err = inflateInit2(&self->gz_stream, -MAX_WBITS);
	if (err < 0) {
		free(self);
		return 0;
	}

	struct mtar_io_in * io2 = malloc(sizeof(struct mtar_io_in));
	io2->ops = &mtar_filter_gzip_in_ops;
	io2->data = self;

	return io2;
}
