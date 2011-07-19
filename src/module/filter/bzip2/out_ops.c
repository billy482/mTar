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
*  Last modified: Tue, 19 Jul 2011 23:04:07 +0200                       *
\***********************************************************************/

// BZ2_bzCompress, BZ2_bzCompressEnd, BZ2_bzCompressInit
#include <bzlib.h>
// free, malloc
#include <malloc.h>

#include <mtar/option.h>

#include "common.h"

struct mtar_filter_bzip2_out {
	bz_stream strm;
	struct mtar_io_out * io;
	short closed;
};

static int mtar_filter_bzip2_out_close(struct mtar_io_out * io);
static int mtar_filter_bzip2_out_flush(struct mtar_io_out * io);
static void mtar_filter_bzip2_out_free(struct mtar_io_out * io);
static int mtar_filter_bzip2_out_last_errno(struct mtar_io_out * io);
static off_t mtar_filter_bzip2_out_pos(struct mtar_io_out * io);
static ssize_t mtar_filter_bzip2_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_filter_bzip2_out_ops = {
	.close      = mtar_filter_bzip2_out_close,
	.flush      = mtar_filter_bzip2_out_flush,
	.free       = mtar_filter_bzip2_out_free,
	.last_errno = mtar_filter_bzip2_out_last_errno,
	.pos        = mtar_filter_bzip2_out_pos,
	.write      = mtar_filter_bzip2_out_write,
};


int mtar_filter_bzip2_out_close(struct mtar_io_out * io) {
	struct mtar_filter_bzip2_out * self = io->data;
	if (self->closed)
		return 0;

	char buffer[1024];
	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	int err;
	do {
		self->strm.next_out = buffer;
		self->strm.avail_out = 1024;
		self->strm.total_out_lo32 = 0;

		err = BZ2_bzCompress(&self->strm, BZ_FINISH);
		if (err >= 0)
			self->io->ops->write(self->io, buffer, self->strm.total_out_lo32);
	} while (err != BZ_STREAM_END);

	BZ2_bzCompressEnd(&self->strm);
	self->closed = 1;

	return self->io->ops->close(self->io);
}

int mtar_filter_bzip2_out_flush(struct mtar_io_out * io) {
	struct mtar_filter_bzip2_out * self = io->data;

	char buffer[1024];

	self->strm.next_in = 0;
	self->strm.avail_in = 0;

	int ok;

	do {
		self->strm.next_out = buffer;
		self->strm.avail_out = 1024;
		self->strm.total_out_lo32 = 0;

		ok = BZ2_bzCompress(&self->strm, BZ_FLUSH);

		self->io->ops->write(self->io, buffer, self->strm.total_out_lo32);
	} while (ok == BZ_FLUSH_OK);

	return self->io->ops->flush(self->io);
}

void mtar_filter_bzip2_out_free(struct mtar_io_out * io) {
	struct mtar_filter_bzip2_out * self = io->data;
	if (!self->closed)
		mtar_filter_bzip2_out_close(io);

	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_bzip2_out_last_errno(struct mtar_io_out * io) {
	struct mtar_filter_bzip2_out * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_bzip2_out_pos(struct mtar_io_out * io) {
	struct mtar_filter_bzip2_out * self = io->data;
	return self->strm.total_in_lo32;
}

ssize_t mtar_filter_bzip2_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_filter_bzip2_out * self = io->data;
	char buff[1024];

	self->strm.next_in = (char *) data;
	self->strm.avail_in = length;

	while (self->strm.avail_in > 0) {
		self->strm.next_out = buff;
		self->strm.avail_out = 1024;
		self->strm.total_out_lo32 = 0;

		int err = BZ2_bzCompress(&self->strm, BZ_RUN);
		if (err == BZ_RUN_OK && self->strm.total_out_lo32 > 0)
			self->io->ops->write(self->io, buff, self->strm.total_out_lo32);
	}

	return length;
}

struct mtar_io_out * mtar_filter_bzip2_new_out(struct mtar_io_out * io, const struct mtar_option * option) {
	struct mtar_filter_bzip2_out * self = malloc(sizeof(struct mtar_filter_bzip2_out));
	self->io = io;
	self->closed = 0;

	// init data
	self->strm.bzalloc = 0;
	self->strm.bzfree = 0;
	self->strm.opaque = 0;

	BZ2_bzCompressInit(&self->strm, option->compress_level, 0, 30);

	struct mtar_io_out * io2 = malloc(sizeof(struct mtar_io_out));
	io2->ops = &mtar_filter_bzip2_out_ops;
	io2->data = self;

	return io2;
}

