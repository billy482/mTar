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
*  Last modified: Tue, 05 Jul 2011 04:43:05 +0200                       *
\***********************************************************************/

// sscanf, snprintf
#include <stdio.h>
// free, malloc, realloc
#include <stdlib.h>
// memcpy, memmove, memset
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


static int mtar_format_ustar_in_check_header(struct mtar_format_ustar * header);
static dev_t mtar_format_ustar_in_convert_dev(struct mtar_format_ustar * header);
static gid_t mtar_format_ustar_in_convert_gid(struct mtar_format_ustar * header);
static ssize_t mtar_format_ustar_in_convert_size(struct mtar_format_ustar * header);
static time_t mtar_format_ustar_in_convert_time(struct mtar_format_ustar * header);
static uid_t mtar_format_ustar_in_convert_uid(struct mtar_format_ustar * header);
static void mtar_format_ustar_in_free(struct mtar_format_in * f);
static int mtar_format_ustar_in_get_header(struct mtar_format_in * f, struct mtar_format_header * header);
static ssize_t mtar_format_ustar_in_prefetch(struct mtar_format_ustar_in * self, ssize_t length);
static ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length);
static ssize_t mtar_format_ustar_in_read_buffer(struct mtar_format_ustar_in * f, void * data, ssize_t length);
static int mtar_format_ustar_in_skip_file(struct mtar_format_in * f);

static struct mtar_format_in_ops mtar_format_ustar_in_ops = {
	.free       = mtar_format_ustar_in_free,
	.get_header = mtar_format_ustar_in_get_header,
	.read       = mtar_format_ustar_in_read,
	.skip_file  = mtar_format_ustar_in_skip_file,
};


int mtar_format_ustar_in_check_header(struct mtar_format_ustar * header) {
	char checksum[8];
	strncpy(checksum, header->checksum, 8);
	memset(header->checksum, ' ', 8);

	unsigned char * ptr = (unsigned char *) header;
	unsigned int i, sum = 0;
	for (i = 0; i < 512; i++)
		sum += ptr[i];
	snprintf(header->checksum, 7, "%06o", sum);

	return strncmp(checksum, header->checksum, 8);
}

dev_t mtar_format_ustar_in_convert_dev(struct mtar_format_ustar * header) {
	unsigned int major = 0, minor = 0;

	sscanf(header->devmajor, "%o", &major);
	sscanf(header->devminor, "%o", &minor);

	return (major << 8 ) | minor;
}

gid_t mtar_format_ustar_in_convert_gid(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->gid, "%o", &result);
	return result;
}

ssize_t mtar_format_ustar_in_convert_size(struct mtar_format_ustar * header) {
	if (header->size[0] == (char) 0x80) {
		short i;
		ssize_t result = 0;
		for (i = 1; i < 12; i++) {
			result <<= 8;
			result |= header->size[i];
		}
		return result;
	} else {
		unsigned long long result = 0;
		sscanf(header->size, "%llo", &result);
		return result;
	}
	return 0;
}

time_t mtar_format_ustar_in_convert_time(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->mtime, "%o", &result);
	return result;
}

uid_t mtar_format_ustar_in_convert_uid(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->uid, "%o", &result);
	return result;
}

void mtar_format_ustar_in_free(struct mtar_format_in * f) {
	struct mtar_format_ustar_in * self = f->data;

	if (self->buffer)
		free(self->buffer);
	self->buffer = 0;

	free(f->data);
	free(f);
}

int mtar_format_ustar_in_get_header(struct mtar_format_in * f, struct mtar_format_header * header) {
	struct mtar_format_ustar_in * self = f->data;
	if (mtar_format_ustar_in_prefetch(self, 2560) < 0)
		return -1;

	mtar_format_init_header(header);
	//char * linkname = 0;

	do {
		char buffer[512];
		struct mtar_format_ustar * h = (struct mtar_format_ustar *) buffer;
		ssize_t nbRead = mtar_format_ustar_in_read_buffer(self, buffer, 512);

		// no header found
		if (nbRead < 512)
			return 1;

		if (mtar_format_ustar_in_check_header(h)) {
			// header checksum failed !!!
		}

		header->dev = mtar_format_ustar_in_convert_dev(h);
		strncpy(header->path, h->filename, 256);
		header->filename = header->path;
		header->size = mtar_format_ustar_in_convert_size(h);
		sscanf(h->filemode, "%o", &header->mode);
		header->mtime = mtar_format_ustar_in_convert_time(h);
		header->uid = mtar_format_ustar_in_convert_uid(h);
		strcpy(header->uname, h->uname);
		header->gid = mtar_format_ustar_in_convert_gid(h);
		strcpy(header->gname, h->gname);
	} while (!header->filename);

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

struct mtar_format_in * mtar_format_ustar_new_in(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
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

int mtar_format_ustar_in_skip_file(struct mtar_format_in * f) {
	return 0;
}

