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
*  Last modified: Sat, 20 Oct 2012 12:45:32 +0200                           *
\***************************************************************************/

// lzma_code, lzma_end, lzma_stream_decoder
#include <lzma.h>
// bool
#include <stdbool.h>
// free
#include <stdlib.h>
// bzero
#include <strings.h>

#include <mtar/verbose.h>

#include "xz.h"

struct mtar_filter_xz_reader {
	lzma_stream strm;
	struct mtar_io_in * io;
	bool closed;

	uint8_t bufferIn[1024];
};

static ssize_t mtar_filter_xz_reader_block_size(struct mtar_io_in * io);
static int mtar_filter_xz_reader_close(struct mtar_io_in * io);
static off_t mtar_filter_xz_reader_forward(struct mtar_io_in * io, off_t offset);
static void mtar_filter_xz_reader_free(struct mtar_io_in * io);
static int mtar_filter_xz_reader_last_errno(struct mtar_io_in * io);
static off_t mtar_filter_xz_reader_position(struct mtar_io_in * io);
static ssize_t mtar_filter_xz_reader_read(struct mtar_io_in * io, void * data, ssize_t length);

static struct mtar_io_in_ops mtar_filter_xz_reader_ops = {
	.block_size = mtar_filter_xz_reader_block_size,
	.close      = mtar_filter_xz_reader_close,
	.forward    = mtar_filter_xz_reader_forward,
	.free       = mtar_filter_xz_reader_free,
	.last_errno = mtar_filter_xz_reader_last_errno,
	.position   = mtar_filter_xz_reader_position,
	.read       = mtar_filter_xz_reader_read,
};


ssize_t mtar_filter_xz_reader_block_size(struct mtar_io_in * io) {
	struct mtar_filter_xz_reader * self = io->data;
	return self->io->ops->block_size(self->io);
}

int mtar_filter_xz_reader_close(struct mtar_io_in * io) {
	struct mtar_filter_xz_reader * self = io->data;
	if (self->closed)
		return 0;

	int failed = self->io->ops->close(self->io);
	if (failed)
		return failed;

	lzma_end(&self->strm);
	self->closed = true;
	return 0;
}

off_t mtar_filter_xz_reader_forward(struct mtar_io_in * io, off_t offset) {
	struct mtar_filter_xz_reader * self = io->data;

	uint8_t buffer[1024];
	self->strm.next_out = buffer;
	self->strm.avail_out = offset > 1024 ? 1024 : offset;
	uint64_t end_pos = self->strm.total_out + offset;

	while (self->strm.total_out < end_pos) {
		while (self->strm.avail_in > 0) {
			lzma_ret err = lzma_code(&self->strm, LZMA_RUN);
			if (err == LZMA_STREAM_END)
				return self->strm.total_out;
			if (self->strm.avail_out == 0) {
				uint64_t tOffset = end_pos - self->strm.total_out;
				if (tOffset == 0)
					return self->strm.total_out;
				self->strm.next_out = buffer;
				self->strm.avail_out = tOffset > 1024 ? 1024 : tOffset;
			}
		}

		int nb_read = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nb_read > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nb_read;
		} else if (nb_read == 0) {
			return self->strm.total_out;
		} else {
			return nb_read;
		}
	}

	return self->strm.total_out;
}

void mtar_filter_xz_reader_free(struct mtar_io_in * io) {
	struct mtar_filter_xz_reader * self = io->data;
	if (!self->closed)
		mtar_filter_xz_reader_close(io);

	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_xz_reader_last_errno(struct mtar_io_in * io) {
	struct mtar_filter_xz_reader * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_xz_reader_position(struct mtar_io_in * io) {
	struct mtar_filter_xz_reader * self = io->data;
	return self->strm.total_out;
}

ssize_t mtar_filter_xz_reader_read(struct mtar_io_in * io, void * data, ssize_t length) {
	struct mtar_filter_xz_reader * self = io->data;

	self->strm.next_out = data;
	self->strm.avail_out = length;
	uint64_t previous_pos = self->strm.total_out;

	while (self->strm.avail_out > 0) {
		if (self->strm.avail_in > 0) {
			lzma_ret err = lzma_code(&self->strm, LZMA_RUN);
			if (err == LZMA_STREAM_END || self->strm.avail_out == 0)
				return self->strm.total_out - previous_pos;
		}

		ssize_t nb_read = self->io->ops->read(self->io, self->bufferIn + self->strm.avail_in, 1024 - self->strm.avail_in);
		if (nb_read > 0) {
			self->strm.next_in = self->bufferIn;
			self->strm.avail_in += nb_read;
		} else if (nb_read == 0) {
			return self->strm.total_out - previous_pos;
		} else {
			return nb_read;
		}
	}

	return self->strm.total_out - previous_pos;
}

struct mtar_io_in * mtar_filter_xz_new_in(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_xz_reader * self = malloc(sizeof(struct mtar_filter_xz_reader));
	self->io = io;
	self->closed = false;

	// init data
	bzero(&self->strm, sizeof(lzma_stream));
	lzma_ret ret = lzma_stream_decoder(&self->strm, UINT64_MAX, 0);
	if (ret != LZMA_OK) {
		free(self);
		return NULL;
	}

	struct mtar_io_in * io2 = malloc(sizeof(struct mtar_io_in));
	io2->ops = &mtar_filter_xz_reader_ops;
	io2->data = self;

	return io2;
}

