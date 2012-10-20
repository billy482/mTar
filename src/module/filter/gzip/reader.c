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
*  Last modified: Sat, 20 Oct 2012 13:18:42 +0200                           *
\***************************************************************************/

// free, malloc
#include <stdlib.h>
// inflate, inflateEnd, inflateInit2
#include <zlib.h>

#include "gzip.h"

struct mtar_filter_gzip_reader {
	z_stream gz_stream;
	struct mtar_io_reader * io;
	short closed;

	char bufferIn[1024];
};

static ssize_t mtar_filter_gzip_reader_block_size(struct mtar_io_reader * io);
static int mtar_filter_gzip_reader_close(struct mtar_io_reader * io);
static off_t mtar_filter_gzip_reader_forward(struct mtar_io_reader * io, off_t offset);
static void mtar_filter_gzip_reader_free(struct mtar_io_reader * io);
static int mtar_filter_gzip_reader_last_errno(struct mtar_io_reader * io);
static off_t mtar_filter_gzip_reader_position(struct mtar_io_reader * io);
static ssize_t mtar_filter_gzip_reader_read(struct mtar_io_reader * io, void * data, ssize_t length);

static struct mtar_io_reader_ops mtar_filter_gzip_reader_ops = {
	.block_size = mtar_filter_gzip_reader_block_size,
	.close      = mtar_filter_gzip_reader_close,
	.forward    = mtar_filter_gzip_reader_forward,
	.free       = mtar_filter_gzip_reader_free,
	.last_errno = mtar_filter_gzip_reader_last_errno,
	.position   = mtar_filter_gzip_reader_position,
	.read       = mtar_filter_gzip_reader_read,
};


ssize_t mtar_filter_gzip_reader_block_size(struct mtar_io_reader * io) {
	struct mtar_filter_gzip_reader * self = io->data;
	return self->io->ops->block_size(self->io);
}

int mtar_filter_gzip_reader_close(struct mtar_io_reader * io) {
	struct mtar_filter_gzip_reader * self = io->data;
	if (self->closed)
		return 0;

	int failed = self->io->ops->close(self->io);
	if (failed)
		return failed;

	inflateEnd(&self->gz_stream);
	self->closed = 1;
	return 0;
}

off_t mtar_filter_gzip_reader_forward(struct mtar_io_reader * io, off_t offset) {
	struct mtar_filter_gzip_reader * self = io->data;

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

		int nb_read = self->io->ops->read(self->io, self->bufferIn + self->gz_stream.avail_in, 1024 - self->gz_stream.avail_in);
		if (nb_read > 0) {
			self->gz_stream.next_in = (unsigned char *) self->bufferIn;
			self->gz_stream.avail_in += nb_read;
		} else if (nb_read == 0) {
			return self->gz_stream.total_out;
		} else {
			return nb_read;
		}
	}

	return self->gz_stream.total_out;
}

void mtar_filter_gzip_reader_free(struct mtar_io_reader * io) {
	struct mtar_filter_gzip_reader * self = io->data;
	if (!self->closed)
		mtar_filter_gzip_reader_close(io);

	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_gzip_reader_last_errno(struct mtar_io_reader * io) {
	struct mtar_filter_gzip_reader * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_gzip_reader_position(struct mtar_io_reader * io) {
	struct mtar_filter_gzip_reader * self = io->data;
	return self->gz_stream.total_out;
}

ssize_t mtar_filter_gzip_reader_read(struct mtar_io_reader * io, void * data, ssize_t length) {
	struct mtar_filter_gzip_reader * self = io->data;

	self->gz_stream.next_out = data;
	self->gz_stream.avail_out = length;
	uLong previous_pos = self->gz_stream.total_out;

	while (self->gz_stream.avail_out > 0) {
		if (self->gz_stream.avail_in > 0) {
			int err = inflate(&self->gz_stream, Z_SYNC_FLUSH);
			if (err == Z_STREAM_END || self->gz_stream.avail_out == 0)
				return self->gz_stream.total_out - previous_pos;
		}

		int nb_read = self->io->ops->read(self->io, self->bufferIn + self->gz_stream.avail_in, 1024 - self->gz_stream.avail_in);
		if (nb_read > 0) {
			self->gz_stream.next_in = (unsigned char *) self->bufferIn;
			self->gz_stream.avail_in += nb_read;
		} else if (nb_read == 0) {
			return self->gz_stream.total_out - previous_pos;
		} else {
			return nb_read;
		}
	}

	return self->gz_stream.total_out - previous_pos;
}

struct mtar_io_reader * mtar_filter_gzip_new_reader(struct mtar_io_reader * io, const struct mtar_option * option __attribute__((unused))) {
	struct gzip_header header;
	ssize_t nb_read = io->ops->read(io, &header, sizeof(header));

	if (nb_read < 10 || header.magic[0] != 0x1F || header.magic[1] != 0x8B)
		return 0;

	if (header.flag & gzip_flag_extra_field) {
		unsigned short extra_size;
		nb_read = io->ops->read(io, &extra_size, sizeof(extra_size));

		if (nb_read < 2)
			return 0;

		io->ops->forward(io, extra_size);
	}

	if (header.flag & gzip_flag_name) {
		char c;
		do {
			nb_read = io->ops->read(io, &c, 1);
		} while (c && nb_read > 0);
	}

	if (header.flag & gzip_flag_comment) {
		char c;
		do {
			nb_read = io->ops->read(io, &c, 1);
		} while (c && nb_read > 0);
	}

	if (header.flag & gzip_flag_header_crc16)
		io->ops->forward(io, 2);

	struct mtar_filter_gzip_reader * self = malloc(sizeof(struct mtar_filter_gzip_reader));
	self->io = io;
	self->closed = 0;

	// init data
	self->gz_stream.zalloc = 0;
	self->gz_stream.zfree = 0;
	self->gz_stream.opaque = 0;

	self->gz_stream.next_in = 0;
	self->gz_stream.avail_in = 0;
	self->gz_stream.total_in = 0;

	self->gz_stream.total_out = 0;

	int err = inflateInit2(&self->gz_stream, -MAX_WBITS);
	if (err < 0) {
		free(self);
		return 0;
	}

	struct mtar_io_reader * io2 = malloc(sizeof(struct mtar_io_reader));
	io2->ops = &mtar_filter_gzip_reader_ops;
	io2->data = self;

	return io2;
}

