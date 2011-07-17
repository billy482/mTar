/***********************************************************************\
*       ______           ____     ___           __   _                  *
*      / __/ /____  ____/  _/__ _/ _ | ________/ /  (_)  _____ ____     *
*     _\ \/ __/ _ \/ __// // _ `/ __ |/ __/ __/ _ \/ / |/ / -_) __/     *
*    /___/\__/\___/_/ /___/\_, /_/ |_/_/  \__/_//_/_/|___/\__/_/        *
*                           /_/                                         *
*  -------------------------------------------------------------------  *
*  This file is a part of StorIqArchiver                                *
*                                                                       *
*  StorIqArchiver is free software; you can redistribute it and/or      *
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
*  Copyright (C) 2010, Clercin guillaume <gclercin@intellique.com>      *
*  Last modified: Sun, 17 Jul 2011 22:06:47 +0200                       *
\***********************************************************************/

// free, malloc
#include <malloc.h>
// snprintf
#include <stdio.h>
// deflate, deflateEnd, deflateInit
#include <zlib.h>

#include <mtar/option.h>

#include "common.h"


struct mtar_filter_gzip_out {
	z_stream gz_stream;
	struct mtar_io_out * io;
	uLong crc32;
};

static int mtar_filter_gzip_out_close(struct mtar_io_out * io);
static int mtar_filter_gzip_out_flush(struct mtar_io_out * io);
static void mtar_filter_gzip_out_free(struct mtar_io_out * io);
static int mtar_filter_gzip_out_last_errno(struct mtar_io_out * io);
static off_t mtar_filter_gzip_out_pos(struct mtar_io_out * io);
static ssize_t mtar_filter_gzip_out_write(struct mtar_io_out * io, const void * data, ssize_t length);

static struct mtar_io_out_ops mtar_filter_gzip_out_ops = {
	.close      = mtar_filter_gzip_out_close,
	.flush      = mtar_filter_gzip_out_flush,
	.free       = mtar_filter_gzip_out_free,
	.last_errno = mtar_filter_gzip_out_last_errno,
	.pos        = mtar_filter_gzip_out_pos,
	.write      = mtar_filter_gzip_out_write,
};


int mtar_filter_gzip_out_close(struct mtar_io_out * io) {
	struct mtar_filter_gzip_out * self = io->data;

	char buffer[1024];
	self->gz_stream.next_in = 0;
	self->gz_stream.avail_in = 0;

	int err;
	do {
		self->gz_stream.next_out = (unsigned char *) buffer;
		self->gz_stream.avail_out = 1024;
		self->gz_stream.total_out = 0;

		err = deflate(&self->gz_stream, Z_FINISH);
		if (err >= 0)
			self->io->ops->write(self->io, buffer, self->gz_stream.total_out);
	} while (err == Z_OK);

	self->io->ops->write(self->io, &self->crc32, 4);
	unsigned int length = self->gz_stream.total_in;
	self->io->ops->write(self->io, &length, 4);

	deflateEnd(&self->gz_stream);

	return self->io->ops->close(self->io);
}

int mtar_filter_gzip_out_flush(struct mtar_io_out * io) {
	struct mtar_filter_gzip_out * self = io->data;

	char buffer[1024];

	self->gz_stream.next_in = 0;
	self->gz_stream.avail_in = 0;

	self->gz_stream.next_out = (unsigned char *) buffer;
	self->gz_stream.avail_out = 1024;
	self->gz_stream.total_out = 0;

	deflate(&self->gz_stream, Z_FULL_FLUSH);

	self->io->ops->write(self->io, buffer, self->gz_stream.total_out);

	return self->io->ops->flush(self->io);
}

void mtar_filter_gzip_out_free(struct mtar_io_out * io) {
	struct mtar_filter_gzip_out * self = io->data;
	self->io->ops->free(self->io);
	free(self);
	free(io);
}

int mtar_filter_gzip_out_last_errno(struct mtar_io_out * io) {
	struct mtar_filter_gzip_out * self = io->data;
	return self->io->ops->last_errno(self->io);
}

off_t mtar_filter_gzip_out_pos(struct mtar_io_out * io) {
	struct mtar_filter_gzip_out * self = io->data;
	return self->io->ops->pos(self->io);
}

ssize_t mtar_filter_gzip_out_write(struct mtar_io_out * io, const void * data, ssize_t length) {
	struct mtar_filter_gzip_out * self = io->data;
	char buff[1024];

	self->gz_stream.next_in = (unsigned char *) data;
	self->gz_stream.avail_in = length;

	self->crc32 = crc32(self->crc32, data, length);

	while (self->gz_stream.avail_in > 0) {
		self->gz_stream.next_out = (unsigned char *) buff;
		self->gz_stream.avail_out = 1024;
		self->gz_stream.total_out = 0;

		int err = deflate(&self->gz_stream, Z_NO_FLUSH);
		if (err >= 0 && self->gz_stream.total_out > 0)
			self->io->ops->write(self->io, buff, self->gz_stream.total_out);
	}

	return length;
}

struct mtar_io_out * mtar_filter_gzip_new_out(struct mtar_io_out * io, const struct mtar_option * option __attribute__((unused))) {
	struct mtar_filter_gzip_out * self = malloc(sizeof(struct mtar_filter_gzip_out));
	self->io = io;

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

	int err = deflateInit2(&self->gz_stream, option->compress_level, Z_DEFLATED, -MAX_WBITS, 9, Z_DEFAULT_STRATEGY);
	if (err) {
		free(self);
		return 0;
	}

	// write header
	char header[11];
	snprintf(header, 11, "%c%c%c%c%c%c%c%c%c%c", 0x1f, 0x8b, 0x8, 0, 0, 0, 0, 0, self->gz_stream.data_type, 3);
	io->ops->write(io, header, 10);

	struct mtar_io_out * io2 = malloc(sizeof(struct mtar_io_out));
	io2->ops = &mtar_filter_gzip_out_ops;
	io2->data = self;

	return io2;
}

