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
*  Last modified: Mon, 12 Sep 2011 17:40:17 +0200                       *
\***********************************************************************/

// BZ2_bzDecompress, BZ2_bzDecompressEnd, BZ2_bzDecompressInit
#include <bzlib.h>
// free, malloc
#include <malloc.h>

#include <mtar/verbose.h>

#include "common.h"

struct mtar_filter_bzip2_in {
	bz_stream strm;
	struct mtar_io_in * io;
	short closed;

	char bufferIn[1024];
};

static ssize_t mtar_filter_bzip2_in_block_size(struct mtar_io_in * io);
static int mtar_filter_bzip2_in_close(struct mtar_io_in * io);
static off_t mtar_filter_bzip2_in_forward(struct mtar_io_in * io, off_t offset);
static void mtar_filter_bzip2_in_free(struct mtar_io_in * io);
static int mtar_filter_bzip2_in_last_errno(struct mtar_io_in * io);
static off_t mtar_filter_bzip2_in_pos(struct mtar_io_in * io);
static ssize_t mtar_filter_bzip2_in_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_filter_bzip2_in_ops = {
	.block_size = mtar_filter_bzip2_in_block_size,
	.close      = mtar_filter_bzip2_in_close,
	.forward    = mtar_filter_bzip2_in_forward,
	.free       = mtar_filter_bzip2_in_free,
	.last_errno = mtar_filter_bzip2_in_last_errno,
	.pos        = mtar_filter_bzip2_in_pos,
	.read       = mtar_filter_bzip2_in_read,
};


ssize_t mtar_filter_bzip2_in_block_size(struct mtar_io_in * io) {
	struct mtar_filter_bzip2_in * self = io->data;
	return self->io->ops->block_size(self->io);
}

int mtar_filter_bzip2_in_close(struct mtar_io_in * io) {
	struct mtar_filter_bzip2_in * self = io->data;
	if (self->closed)
		return 0;

	int failed = self->io->ops->close(self->io);
	if (failed)
		return failed;

	BZ2_bzDecompressEnd(&self->strm);
	self->closed = 1;
	return 0;
}

off_t mtar_filter_bzip2_in_forward(struct mtar_io_in * io, off_t offset) {
	struct mtar_filter_bzip2_in * self = io->data;

	char buffer[1024];
	self->strm.next_out = buffer;
	self->strm.avail_out = offset > 1024 ? 1024 : offset;
	unsigned int end_pos = self->strm.total_out_lo32 + offset;

	while (self->strm.total_out_lo32 < end_pos) {
		while (self->strm.avail_in > 0) {
			int err = BZ2_bzDecompress(&self->strm);
			if (err == BZ_STREAM_END)
				return self->strm.total_out_lo32;
			if (self->strm.avail_out == 0) {
				unsigned int tOffset = end_pos - self->strm.total_out_lo32;
				if (tOffset == 0)
					return self->strm.total_out_lo32;
				self->strm.next_out = buffer;
				self->strm.avail_out = tOffset > 1024 ? 1024 : tOffset;
			}
		}

		int nbRead = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nbRead > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nbRead;
		} else if (nbRead == 0) {
			return self->strm.total_out_lo32;
		} else {
			return nbRead;
		}
	}

	return self->strm.total_out_lo32;
}

void mtar_filter_bzip2_in_free(struct mtar_io_in * io) {
	struct mtar_filter_bzip2_in * self = io->data;
	if (!self->closed)
		mtar_filter_bzip2_in_close(io);

	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_bzip2_in_last_errno(struct mtar_io_in * io) {
	struct mtar_filter_bzip2_in * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_bzip2_in_pos(struct mtar_io_in * io) {
	struct mtar_filter_bzip2_in * self = io->data;
	return self->strm.total_out_lo32;
}

ssize_t mtar_filter_bzip2_in_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_filter_bzip2_in * self = io->data;

	self->strm.next_out = data;
	self->strm.avail_out = length;
	unsigned int previous_pos = self->strm.total_out_lo32;

	while (self->strm.avail_out > 0) {
		if (self->strm.avail_in > 0) {
			int err = BZ2_bzDecompress(&self->strm);
			if (err == BZ_STREAM_END || self->strm.avail_out == 0)
				return self->strm.total_out_lo32 - previous_pos;
		}

		int nbRead = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nbRead > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nbRead;
		} else if (nbRead == 0) {
			return self->strm.total_out_lo32 - previous_pos;
		} else {
			return nbRead;
		}
	}

	return self->strm.total_out_lo32 - previous_pos;
}

struct mtar_io_in * mtar_filter_bzip2_new_in(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_bzip2_in * self = malloc(sizeof(struct mtar_filter_bzip2_in));
	self->io = io;
	self->closed = 0;

	// init data
	self->strm.bzalloc = 0;
	self->strm.bzfree = 0;
	self->strm.opaque = 0;
	self->strm.next_in = 0;
	self->strm.avail_in = 0;
	self->strm.total_in_lo32 = 0;
	self->strm.total_in_hi32 = 0;

	BZ2_bzDecompressInit(&self->strm, 0, 0);

	struct mtar_io_in * io2 = malloc(sizeof(struct mtar_io_in));
	io2->ops = &mtar_filter_bzip2_in_ops;
	io2->data = self;

	return io2;
}

