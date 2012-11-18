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
*  Last modified: Sun, 18 Nov 2012 11:02:47 +0100                           *
\***************************************************************************/

// BZ2_bzCompress, BZ2_bzCompressEnd, BZ2_bzCompressInit
#include <bzlib.h>
// bool
#include <stdbool.h>
// free, getsubopt, malloc
#include <stdlib.h>
// sscanf
#include <stdio.h>
// strdup
#include <string.h>
// bzero
#include <strings.h>

#include <mtar/option.h>

#include "bzip2.h"

struct mtar_filter_bzip2_writer {
	bz_stream strm;

	struct mtar_io_writer * io;
	bool closed;

	char * buffer;
	ssize_t buffer_size;
	ssize_t block_size;
};

static ssize_t mtar_filter_bzip2_writer_available_space(struct mtar_io_writer * io);
static ssize_t mtar_filter_bzip2_writer_block_size(struct mtar_io_writer * io);
static int mtar_filter_bzip2_writer_close(struct mtar_io_writer * io);
static bool mtar_filter_bzip2_writer_finish(struct mtar_filter_bzip2_writer * io);
static int mtar_filter_bzip2_writer_flush(struct mtar_io_writer * io);
static void mtar_filter_bzip2_writer_free(struct mtar_io_writer * io);
static int mtar_filter_bzip2_writer_last_errno(struct mtar_io_writer * io);
static ssize_t mtar_filter_bzip2_writer_next_prefered_size(struct mtar_io_writer * io);
static off_t mtar_filter_bzip2_writer_position(struct mtar_io_writer * io);
static struct mtar_io_reader * mtar_filter_bzip2_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option);
static ssize_t mtar_filter_bzip2_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length);

static struct mtar_io_writer_ops mtar_filter_bzip2_writer_ops = {
	.available_space    = mtar_filter_bzip2_writer_available_space,
	.block_size         = mtar_filter_bzip2_writer_block_size,
	.close              = mtar_filter_bzip2_writer_close,
	.flush              = mtar_filter_bzip2_writer_flush,
	.free               = mtar_filter_bzip2_writer_free,
	.last_errno         = mtar_filter_bzip2_writer_last_errno,
	.next_prefered_size = mtar_filter_bzip2_writer_next_prefered_size,
	.position           = mtar_filter_bzip2_writer_position,
	.reopen_for_reading = mtar_filter_bzip2_writer_reopen_for_reading,
	.write              = mtar_filter_bzip2_writer_write,
};


static ssize_t mtar_filter_bzip2_writer_available_space(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	ssize_t available = self->io->ops->available_space(self->io);
	return available > self->block_size || available < 0 ? available : 0;
}

static ssize_t mtar_filter_bzip2_writer_block_size(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	return self->block_size;
}

static int mtar_filter_bzip2_writer_close(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	if (self->closed)
		return 0;

	if (mtar_filter_bzip2_writer_finish(self))
		return 1;

	return self->io->ops->close(self->io);
}

static bool mtar_filter_bzip2_writer_finish(struct mtar_filter_bzip2_writer * self) {
	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	int err = 0;
	do {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out_hi32 = self->strm.total_out_lo32 = 0;

		err = BZ2_bzCompress(&self->strm, BZ_FINISH);
		if (err < 0 || self->io->ops->write(self->io, self->buffer, self->strm.total_out_lo32) < 0)
			return true;
	} while (err != BZ_STREAM_END);

	BZ2_bzCompressEnd(&self->strm);
	self->closed = true;

	return false;
}

static int mtar_filter_bzip2_writer_flush(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;

	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	int ok;
	do {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out_hi32 = self->strm.total_out_lo32 = 0;

		ok = BZ2_bzCompress(&self->strm, BZ_FLUSH);
		if (ok < 0 || self->io->ops->write(self->io, self->buffer, self->strm.total_out_lo32) < 0)
			return 1;
	} while (ok == BZ_FLUSH_OK);

	return self->io->ops->flush(self->io);
}

static void mtar_filter_bzip2_writer_free(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	if (!self->closed)
		mtar_filter_bzip2_writer_close(io);

	self->io->ops->free(self->io);
	free(self->buffer);
	free(self);
	free(io);
}

static int mtar_filter_bzip2_writer_last_errno(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	return self->io->ops->last_errno(self->io);
}

static ssize_t mtar_filter_bzip2_writer_next_prefered_size(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	ssize_t next = self->io->ops->next_prefered_size(self->io);
	if (next < self->block_size && self->io->ops->available_space(self->io) < self->block_size)
		return 0;
	return self->block_size;
}

static off_t mtar_filter_bzip2_writer_position(struct mtar_io_writer * io) {
	struct mtar_filter_bzip2_writer * self = io->data;
	off_t offset = self->strm.total_in_hi32;
	return self->strm.total_in_lo32 | (offset << 32);
}

static struct mtar_io_reader * mtar_filter_bzip2_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_filter_bzip2_writer * self = io->data;

	if (self->closed || mtar_filter_bzip2_writer_finish(self))
		return NULL;

	struct mtar_io_reader * in = self->io->ops->reopen_for_reading(self->io, option);
	if (in != NULL)
		return mtar_filter_bzip2_new_reader(in, option, NULL);

	return NULL;
}

static ssize_t mtar_filter_bzip2_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length) {
	struct mtar_filter_bzip2_writer * self = io->data;

	self->strm.next_in = (char *) data;
	self->strm.avail_in = length;

	while (self->strm.avail_in > 0) {
		self->strm.next_out = self->buffer;
		self->strm.avail_out = self->io->ops->next_prefered_size(self->io);
		self->strm.total_out_hi32 = self->strm.total_out_lo32 = 0;

		int err = BZ2_bzCompress(&self->strm, BZ_RUN);
		if (err == BZ_RUN_OK && self->strm.total_out_lo32 > 0 && self->io->ops->write(self->io, self->buffer, self->strm.total_out_lo32) < 0)
			return -1;
	}

	ssize_t available = self->io->ops->available_space(self->io);
	if (available < 2 * self->block_size)
		mtar_filter_bzip2_writer_flush(io);

	return length;
}

struct mtar_io_writer * mtar_filter_bzip2_new_writer(struct mtar_io_writer * io, const struct mtar_option * option, const char * parameters) {
	int level = option->compress_level;

	if (parameters != NULL) {
		enum {
			compression_level = 0,
		};
		char * const token[] = {
			[compression_level] = "compression-level",
			NULL,
		};

		char * param = strdup(parameters);
		size_t sparam = strlen(param);
		char * value;
		while (*param != '\0') {
			switch (getsubopt(&param, token, &value)) {
				case compression_level:
					if (value != NULL) {
						int val = 6;
						sscanf(value, "%d", &val);

						level = val >= 0 && val <= 9 ? val : 6;
					} else {
						level = 6;
					}
					break;
			}
		}
		free(param - sparam);
	}

	struct mtar_filter_bzip2_writer * self = malloc(sizeof(struct mtar_filter_bzip2_writer));
	self->io = io;
	self->closed = 0;
	self->buffer_size = io->ops->block_size(io);
	self->buffer = malloc(self->buffer_size);
	self->block_size = (level * 100) << 10;

	// init data
	bzero(&self->strm, sizeof(self->strm));
	BZ2_bzCompressInit(&self->strm, level, 0, 30);

	struct mtar_io_writer * io2 = malloc(sizeof(struct mtar_io_writer));
	io2->ops = &mtar_filter_bzip2_writer_ops;
	io2->data = self;

	return io2;
}

