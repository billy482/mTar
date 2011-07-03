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
*  Last modified: Sun, 03 Jul 2011 14:39:36 +0200                       *
\***********************************************************************/

// free, malloc, realloc
#include <stdlib.h>
// memcpy, memmove
#include <string.h>

#include <mtar/io.h>

#include "common.h"

struct mtar_format_ustar_in {
	struct mtar_io_in * io;
	off_t position;
	ssize_t size;
	char * buffer;
	unsigned int bufferSize;
	unsigned int bufferUsed;
};


static void mtar_format_ustar_in_free(struct mtar_format_in * f);
static int mtar_format_ustar_in_getHeader(struct mtar_format_in * f, struct mtar_format_header * header);
static ssize_t mtar_format_ustar_in_prefetch(struct mtar_format_ustar_in * self, ssize_t length);
static ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length);
static ssize_t mtar_format_ustar_in_read_buffer(struct mtar_format_ustar_in * f, void * data, ssize_t length);

static struct mtar_format_in_ops mtar_format_ustar_in_ops = {
	.free      = mtar_format_ustar_in_free,
	.getHeader = mtar_format_ustar_in_getHeader,
	.read      = mtar_format_ustar_in_read,
};


void mtar_format_ustar_in_free(struct mtar_format_in * f) {
	struct mtar_format_ustar_in * self = f->data;

	if (self->buffer)
		free(self->buffer);
	self->buffer = 0;

	free(f->data);
	free(f);
}

int mtar_format_ustar_in_getHeader(struct mtar_format_in * f, struct mtar_format_header * header) {
	struct mtar_format_ustar_in * self = f->data;
	if (mtar_format_ustar_in_prefetch(self, 2560) < 0)
		return -1;

	header->filename = 0;
	char * linkname = 0;

	do {
		char buffer[512];
		ssize_t nbRead = mtar_format_ustar_in_read_buffer(self, buffer, 512);

		struct mtar_format_ustar * h = buffer;
		strncpy(header->path, h->filename, 256);
		header->filename = header->path;

	} while (header->filename);

	return 0;
}

ssize_t mtar_format_ustar_in_prefetch(struct mtar_format_ustar_in * self, ssize_t length) {
	if (self->bufferSize < length) {
		self->buffer = realloc(self->buffer, length);
		self->bufferSize = length;
	}
	if (self->bufferUsed < length) {
		ssize_t nbRead = self->io->ops->read(self->io, self->buffer + self->bufferUsed, length - self->bufferUsed);
		if (nbRead < 0)
			return nbRead;
		if (nbRead > 0)
			self->bufferUsed += nbRead;
	}
	return self->bufferUsed;
}

ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length) {
	return 0;
}

ssize_t mtar_format_ustar_in_read_buffer(struct mtar_format_ustar_in * self, void * data, ssize_t length) {
	ssize_t nbRead = 0;
	if (self->bufferUsed > 0) {
		nbRead = length > self->bufferUsed ? self->bufferUsed : length;
		memcpy(data, self->buffer, nbRead);

		self->bufferUsed -= nbRead;
		if (self->bufferUsed > 0)
			memmove(self->buffer, self->buffer + nbRead, self->bufferUsed);

		if (length == nbRead) {
			self->position += nbRead;
			return nbRead;
		}
	}

	ssize_t r = self->io->ops->read(self->io, data + nbRead, length - nbRead);
	if (r < 0) {
		memcpy(self->buffer, data, nbRead);
		self->bufferUsed = nbRead;
		return r;
	}

	if (self->bufferUsed == 0) {
		free(self->buffer);
		self->buffer = 0;
		self->bufferSize = 0;
	}

	self->position += nbRead;
	return nbRead;
}

struct mtar_format_in * mtar_format_ustar_newIn(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	if (!io)
		return 0;

	struct mtar_format_ustar_in * data = malloc(sizeof(struct mtar_format_ustar_in));
	data->io = io;
	data->position = 0;
	data->size = 0;
	data->buffer = 0;
	data->bufferSize = 0;
	data->bufferUsed = 0;

	struct mtar_format_in * self = malloc(sizeof(struct mtar_format_in));
	self->ops = &mtar_format_ustar_in_ops;
	self->data = data;

	return self;
}

