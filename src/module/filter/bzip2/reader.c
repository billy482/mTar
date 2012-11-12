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
*  Last modified: Mon, 12 Nov 2012 12:21:49 +0100                           *
\***************************************************************************/

// BZ2_bzDecompress, BZ2_bzDecompressEnd, BZ2_bzDecompressInit
#include <bzlib.h>
// bool
#include <stdbool.h>
// free, malloc
#include <stdlib.h>
// bzero
#include <strings.h>

#include <mtar/verbose.h>

#include "bzip2.h"

struct mtar_filter_bzip2_reader {
	bz_stream strm;
	struct mtar_io_reader * io;
	bool closed;

	char bufferIn[1024];
};

static ssize_t mtar_filter_bzip2_reader_block_size(struct mtar_io_reader * io);
static int mtar_filter_bzip2_reader_close(struct mtar_io_reader * io);
static off_t mtar_filter_bzip2_reader_convert_total_out(bz_stream * strm);
static off_t mtar_filter_bzip2_reader_forward(struct mtar_io_reader * io, off_t offset);
static void mtar_filter_bzip2_reader_free(struct mtar_io_reader * io);
static int mtar_filter_bzip2_reader_last_errno(struct mtar_io_reader * io);
static off_t mtar_filter_bzip2_reader_position(struct mtar_io_reader * io);
static ssize_t mtar_filter_bzip2_reader_read(struct mtar_io_reader * io, void * data, ssize_t length);

static struct mtar_io_reader_ops mtar_filter_bzip2_reader_ops = {
	.block_size = mtar_filter_bzip2_reader_block_size,
	.close      = mtar_filter_bzip2_reader_close,
	.forward    = mtar_filter_bzip2_reader_forward,
	.free       = mtar_filter_bzip2_reader_free,
	.last_errno = mtar_filter_bzip2_reader_last_errno,
	.position   = mtar_filter_bzip2_reader_position,
	.read       = mtar_filter_bzip2_reader_read,
};


static ssize_t mtar_filter_bzip2_reader_block_size(struct mtar_io_reader * io) {
	struct mtar_filter_bzip2_reader * self = io->data;
	return self->io->ops->block_size(self->io);
}

static int mtar_filter_bzip2_reader_close(struct mtar_io_reader * io) {
	struct mtar_filter_bzip2_reader * self = io->data;
	if (self->closed)
		return 0;

	int failed = self->io->ops->close(self->io);
	if (failed)
		return failed;

	BZ2_bzDecompressEnd(&self->strm);
	self->closed = true;
	return 0;
}

static off_t mtar_filter_bzip2_reader_convert_total_out(bz_stream * strm) {
	off_t offset = strm->total_out_hi32;
	return strm->total_out_lo32 | (offset << 32);
}

static off_t mtar_filter_bzip2_reader_forward(struct mtar_io_reader * io, off_t offset) {
	struct mtar_filter_bzip2_reader * self = io->data;
	if (self->closed)
		return mtar_filter_bzip2_reader_convert_total_out(&self->strm);

	char buffer[1024];
	self->strm.next_out = buffer;
	self->strm.avail_out = offset > 1024 ? 1024 : offset;
	off_t end_pos = mtar_filter_bzip2_reader_convert_total_out(&self->strm) + offset;

	while (mtar_filter_bzip2_reader_convert_total_out(&self->strm) < end_pos) {
		while (self->strm.avail_in > 0) {
			int err = BZ2_bzDecompress(&self->strm);
			if (err == BZ_STREAM_END)
				return mtar_filter_bzip2_reader_convert_total_out(&self->strm);

			if (self->strm.avail_out == 0) {
				off_t remain = end_pos - mtar_filter_bzip2_reader_convert_total_out(&self->strm);
				if (remain == 0)
					return mtar_filter_bzip2_reader_convert_total_out(&self->strm);
				self->strm.next_out = buffer;
				self->strm.avail_out = remain > 1024 ? 1024 : remain;
			}
		}

		int nb_read = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nb_read > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nb_read;
		} else if (nb_read == 0) {
			return mtar_filter_bzip2_reader_convert_total_out(&self->strm);
		} else {
			return nb_read;
		}
	}

	return mtar_filter_bzip2_reader_convert_total_out(&self->strm);
}

static void mtar_filter_bzip2_reader_free(struct mtar_io_reader * io) {
	struct mtar_filter_bzip2_reader * self = io->data;
	if (!self->closed)
		mtar_filter_bzip2_reader_close(io);

	self->io->ops->free(self->io);
	free(self);
	free(io);
}

static int mtar_filter_bzip2_reader_last_errno(struct mtar_io_reader * io) {
	struct mtar_filter_bzip2_reader * self = io->data;
	return self->io->ops->last_errno(self->io);
}

static off_t mtar_filter_bzip2_reader_position(struct mtar_io_reader * io) {
	struct mtar_filter_bzip2_reader * self = io->data;
	return mtar_filter_bzip2_reader_convert_total_out(&self->strm);
}

static ssize_t mtar_filter_bzip2_reader_read(struct mtar_io_reader * io, void * data, ssize_t length) {
	struct mtar_filter_bzip2_reader * self = io->data;

	if (self->closed)
		return -1;

	self->strm.next_out = data;
	self->strm.avail_out = length;
	off_t previous_pos = mtar_filter_bzip2_reader_convert_total_out(&self->strm);

	while (self->strm.avail_out > 0) {
		if (self->strm.avail_in > 0) {
			int err = BZ2_bzDecompress(&self->strm);
			if (err == BZ_STREAM_END || self->strm.avail_out == 0)
				return mtar_filter_bzip2_reader_convert_total_out(&self->strm) - previous_pos;
		}

		int nb_read = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nb_read > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nb_read;
		} else if (nb_read == 0) {
			return mtar_filter_bzip2_reader_convert_total_out(&self->strm) - previous_pos;
		} else {
			return nb_read;
		}
	}

	return mtar_filter_bzip2_reader_convert_total_out(&self->strm) - previous_pos;
}

struct mtar_io_reader * mtar_filter_bzip2_new_reader(struct mtar_io_reader * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_bzip2_reader * self = malloc(sizeof(struct mtar_filter_bzip2_reader));
	self->io = io;
	self->closed = false;

	// init data
	bzero(&self->strm, sizeof(self->strm));
	BZ2_bzDecompressInit(&self->strm, 0, 0);

	struct mtar_io_reader * io2 = malloc(sizeof(struct mtar_io_reader));
	io2->ops = &mtar_filter_bzip2_reader_ops;
	io2->data = self;

	return io2;
}

