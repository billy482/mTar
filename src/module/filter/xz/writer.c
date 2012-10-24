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
*  Last modified: Wed, 24 Oct 2012 19:28:03 +0200                           *
\***************************************************************************/

// lzma_code, lzma_easy_encoder, lzma_end
#include <lzma.h>
// bool
#include <stdbool.h>
// free, malloc
#include <stdlib.h>
// bzero
#include <strings.h>

#include <mtar/option.h>

#include "xz.h"

struct mtar_filter_xz_writer {
	lzma_stream strm;

	struct mtar_io_writer * io;
	bool closed;

	uint8_t * buffer;
	ssize_t buffer_size;
	ssize_t block_size;
};

static ssize_t mtar_filter_xz_writer_available_space(struct mtar_io_writer * io);
static ssize_t mtar_filter_xz_writer_block_size(struct mtar_io_writer * io);
static int mtar_filter_xz_writer_close(struct mtar_io_writer * io);
static int mtar_filter_xz_writer_finish(struct mtar_filter_xz_writer * io);
static int mtar_filter_xz_writer_flush(struct mtar_io_writer * io);
static void mtar_filter_xz_writer_free(struct mtar_io_writer * io);
static int mtar_filter_xz_writer_last_errno(struct mtar_io_writer * io);
static ssize_t mtar_filter_xz_writer_next_prefered_size(struct mtar_io_writer * io);
static off_t mtar_filter_xz_writer_position(struct mtar_io_writer * io);
static struct mtar_io_reader * mtar_filter_xz_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option);
static ssize_t mtar_filter_xz_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length);

static struct mtar_io_writer_ops mtar_filter_xz_writer_ops = {
	.available_space    = mtar_filter_xz_writer_available_space,
	.block_size         = mtar_filter_xz_writer_block_size,
	.close              = mtar_filter_xz_writer_close,
	.flush              = mtar_filter_xz_writer_flush,
	.free               = mtar_filter_xz_writer_free,
	.last_errno         = mtar_filter_xz_writer_last_errno,
	.next_prefered_size = mtar_filter_xz_writer_next_prefered_size,
	.position           = mtar_filter_xz_writer_position,
	.reopen_for_reading = mtar_filter_xz_writer_reopen_for_reading,
	.write              = mtar_filter_xz_writer_write,
};


static ssize_t mtar_filter_xz_writer_available_space(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	ssize_t available = self->io->ops->available_space(self->io);
	return available > self->block_size || available < 0 ? available : 0;
}

static ssize_t mtar_filter_xz_writer_block_size(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	return self->block_size;
}

static int mtar_filter_xz_writer_close(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	if (self->closed)
		return 0;

	if (mtar_filter_xz_writer_finish(self))
		return 1;

	return self->io->ops->close(self->io);
}

static int mtar_filter_xz_writer_finish(struct mtar_filter_xz_writer * self) {
	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	lzma_ret err = 0;
	do {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out = 0;

		err = lzma_code(&self->strm, LZMA_FINISH);
		if (!(err == LZMA_STREAM_END || err == LZMA_OK) || self->io->ops->write(self->io, self->buffer, self->strm.total_out) < 0)
			return true;
	} while (err != LZMA_STREAM_END);

	lzma_end(&self->strm);
	self->closed = true;

	return false;
}

static int mtar_filter_xz_writer_flush(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;

	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	lzma_ret ok;
	do {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out = 0;

		ok = lzma_code(&self->strm, LZMA_SYNC_FLUSH);
		if (ok != LZMA_STREAM_END && ok != LZMA_OK)
			return 1;

		if (self->io->ops->write(self->io, self->buffer, self->strm.total_out) < 0)
			return 1;
	} while (ok != LZMA_STREAM_END);

	return self->io->ops->flush(self->io);
}

static void mtar_filter_xz_writer_free(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	if (!self->closed)
		mtar_filter_xz_writer_close(io);

	self->io->ops->free(self->io);
	free(self->buffer);
	free(self);
	free(io);
}

static int mtar_filter_xz_writer_last_errno(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	return self->io->ops->last_errno(self->io);
}

static ssize_t mtar_filter_xz_writer_next_prefered_size(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	ssize_t next = self->io->ops->next_prefered_size(self->io);
	if (next < self->block_size && self->io->ops->available_space(self->io) < self->block_size)
		return 0;
	return self->block_size;
}

static off_t mtar_filter_xz_writer_position(struct mtar_io_writer * io) {
	struct mtar_filter_xz_writer * self = io->data;
	return self->strm.total_in;
}

static struct mtar_io_reader * mtar_filter_xz_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_filter_xz_writer * self = io->data;
	if (self->closed)
		return NULL;

	if (self->closed || mtar_filter_xz_writer_finish(self))
		return NULL;

	struct mtar_io_reader * in = self->io->ops->reopen_for_reading(self->io, option);
	if (in != NULL)
		return mtar_filter_xz_new_reader(in, option);

	return NULL;
}

static ssize_t mtar_filter_xz_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length) {
	struct mtar_filter_xz_writer * self = io->data;

	self->strm.next_in = data;
	self->strm.avail_in = length;

	while (self->strm.avail_in > 0) {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out = 0;

		lzma_ret err = lzma_code(&self->strm, LZMA_RUN);
		if (err == LZMA_OK && self->strm.total_out > 0 && self->io->ops->write(self->io, self->buffer, self->strm.total_out) < 0)
			return -1;
	}

	ssize_t available = self->io->ops->available_space(self->io);
	if (available < 2 * self->block_size)
		mtar_filter_xz_writer_flush(io);

	return length;
}

struct mtar_io_writer * mtar_filter_xz_new_writer(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_filter_xz_writer * self = malloc(sizeof(struct mtar_filter_xz_writer));
	self->io = io;
	self->closed = false;
	self->buffer_size = io->ops->block_size(io);
	self->buffer = malloc(self->buffer_size);
	self->block_size = (option->compress_level * 100) << 10;

	// init data
	bzero(&self->strm, sizeof(lzma_stream));
	lzma_ret ret = lzma_easy_encoder(&self->strm, option->compress_level, LZMA_CHECK_CRC64);
	if (ret != LZMA_OK) {
		free(self);
		return NULL;
	}

	struct mtar_io_writer * io2 = malloc(sizeof(struct mtar_io_writer));
	io2->ops = &mtar_filter_xz_writer_ops;
	io2->data = self;

	return io2;
}

