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
*  Last modified: Mon, 12 Nov 2012 15:01:32 +0100                           *
\***************************************************************************/

// bool
#include <stdbool.h>
// free, malloc
#include <stdlib.h>
// time
#include <time.h>
// crc32, deflate, deflateEnd, deflateInit2
#include <zlib.h>

#include <mtar/option.h>

#include "gzip.h"


struct mtar_filter_gzip_writer {
	z_stream gz_stream;

	struct mtar_io_writer * io;
	uLong crc32;
	bool closed;

	unsigned char * buffer;
	ssize_t buffer_size;
	ssize_t block_size;
};

static ssize_t mtar_filter_gzip_writer_available_space(struct mtar_io_writer * io);
static ssize_t mtar_filter_gzip_writer_block_size(struct mtar_io_writer * io);
static int mtar_filter_gzip_writer_close(struct mtar_io_writer * io);
static bool mtar_filter_gzip_writer_finish(struct mtar_filter_gzip_writer * io);
static int mtar_filter_gzip_writer_flush(struct mtar_io_writer * io);
static void mtar_filter_gzip_writer_free(struct mtar_io_writer * io);
static int mtar_filter_gzip_writer_last_errno(struct mtar_io_writer * io);
static ssize_t mtar_filter_gzip_writer_next_prefered_size(struct mtar_io_writer * io);
static off_t mtar_filter_gzip_writer_position(struct mtar_io_writer * io);
static struct mtar_io_reader * mtar_filter_gzip_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option);
static ssize_t mtar_filter_gzip_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length);

static struct mtar_io_writer_ops mtar_filter_gzip_writer_ops = {
	.available_space    = mtar_filter_gzip_writer_available_space,
	.block_size         = mtar_filter_gzip_writer_block_size,
	.close              = mtar_filter_gzip_writer_close,
	.flush              = mtar_filter_gzip_writer_flush,
	.free               = mtar_filter_gzip_writer_free,
	.last_errno         = mtar_filter_gzip_writer_last_errno,
	.next_prefered_size = mtar_filter_gzip_writer_next_prefered_size,
	.position           = mtar_filter_gzip_writer_position,
	.reopen_for_reading = mtar_filter_gzip_writer_reopen_for_reading,
	.write              = mtar_filter_gzip_writer_write,
};


static ssize_t mtar_filter_gzip_writer_available_space(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	ssize_t available = self->io->ops->available_space(self->io);
	return available > self->block_size || available < 0 ? available : 0;
}

static ssize_t mtar_filter_gzip_writer_block_size(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	return self->block_size;
}

static int mtar_filter_gzip_writer_close(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	if (self->closed)
		return 0;

	if (mtar_filter_gzip_writer_finish(self))
		return 1;

	return self->io->ops->close(self->io);
}

static bool mtar_filter_gzip_writer_finish(struct mtar_filter_gzip_writer * self) {
	self->gz_stream.next_in = 0;
	self->gz_stream.avail_in = 0;

	int err = 0;
	do {
		self->gz_stream.next_out = self->buffer;
		self->gz_stream.avail_out = self->io->ops->next_prefered_size(self->io);
		self->gz_stream.total_out = 0;

		err = deflate(&self->gz_stream, Z_FINISH);
		if (err < 0 || self->io->ops->write(self->io, self->buffer, self->gz_stream.total_out) < 0)
			return true;
	} while (err == Z_OK);

	self->io->ops->write(self->io, &self->crc32, 4);
	unsigned int length = self->gz_stream.total_in;
	self->io->ops->write(self->io, &length, 4);

	deflateEnd(&self->gz_stream);
	self->closed = true;

	return false;
}

static int mtar_filter_gzip_writer_flush(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;

	self->gz_stream.next_in = 0;
	self->gz_stream.avail_in = 0;

	int ok;
	do {
		self->gz_stream.next_out = self->buffer;
		self->gz_stream.avail_out = self->io->ops->next_prefered_size(self->io);
		self->gz_stream.total_out = 0;

		ok = deflate(&self->gz_stream, Z_FULL_FLUSH);
		if (ok < 0 || self->io->ops->write(self->io, self->buffer, self->gz_stream.total_out) < 0)
			return 1;
	} while (ok > 0);

	return self->io->ops->flush(self->io);
}

static void mtar_filter_gzip_writer_free(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	if (!self->closed)
		mtar_filter_gzip_writer_close(io);

	self->io->ops->free(self->io);
	free(self->buffer);
	free(self);
	free(io);
}

static int mtar_filter_gzip_writer_last_errno(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	return self->io->ops->last_errno(self->io);
}

static ssize_t mtar_filter_gzip_writer_next_prefered_size(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	ssize_t next = self->io->ops->next_prefered_size(self->io);
	if (next < self->block_size && self->io->ops->available_space(self->io) < self->block_size)
		return 0;
	return self->block_size;
}

static off_t mtar_filter_gzip_writer_position(struct mtar_io_writer * io) {
	struct mtar_filter_gzip_writer * self = io->data;
	return self->gz_stream.total_in;
}

static struct mtar_io_reader * mtar_filter_gzip_writer_reopen_for_reading(struct mtar_io_writer * io, const struct mtar_option * option) {
	struct mtar_filter_gzip_writer * self = io->data;

	if (self->closed || mtar_filter_gzip_writer_finish(self))
		return NULL;

	struct mtar_io_reader * in = self->io->ops->reopen_for_reading(self->io, option);
	if (in != NULL)
		return mtar_filter_gzip_new_reader(in, option);

	return NULL;
}

static ssize_t mtar_filter_gzip_writer_write(struct mtar_io_writer * io, const void * data, ssize_t length) {
	struct mtar_filter_gzip_writer * self = io->data;

	self->gz_stream.next_in = (unsigned char *) data;
	self->gz_stream.avail_in = length;

	while (self->gz_stream.avail_in > 0) {
		self->gz_stream.next_out = self->buffer;
		self->gz_stream.avail_out = self->io->ops->next_prefered_size(self->io);
		self->gz_stream.total_out = 0;

		int err = deflate(&self->gz_stream, Z_NO_FLUSH);
		if (err >= 0 && self->gz_stream.total_out > 0 && self->io->ops->write(self->io, self->buffer, self->gz_stream.total_out) < 0)
			return -1;
	}

	self->crc32 = crc32(self->crc32, data, length);

	return length;
}

struct mtar_io_writer * mtar_filter_gzip_new_writer(struct mtar_io_writer * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_gzip_writer * self = malloc(sizeof(struct mtar_filter_gzip_writer));
	self->io = io;
	self->closed = false;
	self->buffer_size = io->ops->block_size(io);
	self->buffer = malloc(self->buffer_size);
	self->block_size = (1 << 17) + (1 << (option->compress_level + 2));

	self->gz_stream.zalloc = 0;
	self->gz_stream.zfree = 0;
	self->gz_stream.opaque = 0;

	self->gz_stream.next_in = 0;
	self->gz_stream.total_in = 0;
	self->gz_stream.avail_in = 0;
	self->gz_stream.next_out = 0;
	self->gz_stream.total_out = 0;
	self->gz_stream.avail_out = 0;

	self->crc32 = crc32(0, Z_NULL, 0);

	int err = deflateInit2(&self->gz_stream, option->compress_level, Z_DEFLATED, -MAX_WBITS, option->compress_level, Z_DEFAULT_STRATEGY);
	if (err) {
		free(self);
		return NULL;
	}

	struct gzip_header header = {
		.magic = { 0x1F, 0x8B },
		.compression_method = 0x08,
		.flag = gzip_flag_none,
		.mtime = time(NULL),
		.extra_flag = self->gz_stream.data_type,
		.os = gzip_os_unix,
	};

	io->ops->write(io, &header, sizeof(header));

	struct mtar_io_writer * io2 = malloc(sizeof(struct mtar_io_writer));
	io2->ops = &mtar_filter_gzip_writer_ops;
	io2->data = self;

	return io2;
}

