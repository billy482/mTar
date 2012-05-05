/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Thu, 22 Sep 2011 10:21:37 +0200                           *
\***************************************************************************/

// free, malloc
#include <stdlib.h>
// strchr
#include <string.h>
// inflate, inflateEnd, inflateInit2
#include <zlib.h>

#include "common.h"

struct mtar_filter_gzip_in {
	z_stream gz_stream;
	struct mtar_io_in * io;
	short closed;

	char bufferIn[1024];
};

static ssize_t mtar_filter_gzip_in_block_size(struct mtar_io_in * io);
static int mtar_filter_gzip_in_close(struct mtar_io_in * io);
static off_t mtar_filter_gzip_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_filter_gzip_in_free(struct mtar_io_in * io);
static int mtar_filter_gzip_in_last_errno(struct mtar_io_in * io);
static off_t mtar_filter_gzip_in_pos(struct mtar_io_in * io);
static ssize_t mtar_filter_gzip_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_filter_gzip_in_ops = {
	.block_size = mtar_filter_gzip_in_block_size,
	.close      = mtar_filter_gzip_in_close,
	.forward    = mtar_filter_gzip_in_forward,
	.free       = mtar_filter_gzip_in_free,
	.last_errno = mtar_filter_gzip_in_last_errno,
	.pos        = mtar_filter_gzip_in_pos,
	.read       = mtar_filter_gzip_in_read,
};


ssize_t mtar_filter_gzip_in_block_size(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	return self->io->ops->block_size(self->io);
}

int mtar_filter_gzip_in_close(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	if (self->closed)
		return 0;

	int failed = self->io->ops->close(self->io);
	if (failed)
		return failed;

	inflateEnd(&self->gz_stream);
	self->closed = 1;
	return 0;
}

off_t mtar_filter_gzip_in_forward(struct mtar_io_in * io, off_t offset) {
	struct mtar_filter_gzip_in * self = io->data;

	unsigned char buffer[1024];
	self->gz_stream.next_out = buffer;
	self->gz_stream.avail_out = offset > 1024 ? 1024 : offset;
	uLong end_pos = self->gz_stream.total_out + offset;

	while (self->gz_stream.total_out < end_pos) {
		while (self->gz_stream.avail_in > 0) {
			int err = inflate(&self->gz_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END)
				return self->gz_stream.total_out;
			if (self->gz_stream.avail_out == 0) {
				uLong tOffset = end_pos - self->gz_stream.total_out;
				if (tOffset == 0)
					return self->gz_stream.total_out;
				self->gz_stream.next_out = buffer;
				self->gz_stream.avail_out = tOffset > 1024 ? 1024 : tOffset;
			}
		}

		int nbRead = self->io->ops->read(self->io, self->bufferIn + self->gz_stream.avail_in, 1024 - self->gz_stream.avail_in);
		if (nbRead > 0) {
			self->gz_stream.next_in = (unsigned char *) self->bufferIn;
			self->gz_stream.avail_in += nbRead;
		} else if (nbRead == 0) {
			return self->gz_stream.total_out;
		} else {
			return nbRead;
		}
	}

	return self->gz_stream.total_out;
}

void mtar_filter_gzip_in_free(struct mtar_io_in * io) {
	struct mtar_filter_gzip_in * self = io->data;
	if (!self->closed)
		mtar_filter_gzip_in_close(io);

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
	return self->gz_stream.total_out;
}

ssize_t mtar_filter_gzip_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_filter_gzip_in * self = io->data;

	self->gz_stream.next_out = data;
	self->gz_stream.avail_out = length;
	uLong previous_pos = self->gz_stream.total_out;

	while (self->gz_stream.avail_out > 0) {
		if (self->gz_stream.avail_in > 0) {
			int err = inflate(&self->gz_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END || self->gz_stream.avail_out == 0)
				return self->gz_stream.total_out - previous_pos;
		}

		int nbRead = self->io->ops->read(self->io, self->bufferIn + self->gz_stream.avail_in, 1024 - self->gz_stream.avail_in);
		if (nbRead > 0) {
			self->gz_stream.next_in = (unsigned char *) self->bufferIn;
			self->gz_stream.avail_in += nbRead;
		} else if (nbRead == 0) {
			return self->gz_stream.total_out - previous_pos;
		} else {
			return nbRead;
		}
	}

	return self->gz_stream.total_out - previous_pos;
}

struct mtar_io_in * mtar_filter_gzip_new_in(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_gzip_in * self = malloc(sizeof(struct mtar_filter_gzip_in));
	self->io = io;
	self->closed = 0;

	int nbRead = io->ops->read(io, self->bufferIn, 1024);

	if (nbRead < 10 || self->bufferIn[0] != 0x1f || ((unsigned char) self->bufferIn[1]) != 0x8b) {
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
	self->gz_stream.total_in = 0;

	self->gz_stream.total_out = 0;

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

