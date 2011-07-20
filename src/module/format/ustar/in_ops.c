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
*  Last modified: Wed, 20 Jul 2011 13:05:50 +0200                       *
\***********************************************************************/

// sscanf, snprintf
#include <stdio.h>
// free, malloc, realloc
#include <stdlib.h>
// memcpy, memmove, memset
#include <string.h>
// S_IFREG
#include <sys/stat.h>

#include <mtar/io.h>

#include "common.h"

struct mtar_format_ustar_in {
	struct mtar_io_in * io;
	off_t position;
	char * buffer;
	unsigned int bufferSize;
	unsigned int bufferUsed;
	ssize_t filesize;
	ssize_t skipsize;
};


static int mtar_format_ustar_in_check_header(struct mtar_format_ustar * header);
static dev_t mtar_format_ustar_in_convert_dev(struct mtar_format_ustar * header);
static gid_t mtar_format_ustar_in_convert_gid(struct mtar_format_ustar * header);
static ssize_t mtar_format_ustar_in_convert_size(struct mtar_format_ustar * header);
static time_t mtar_format_ustar_in_convert_time(struct mtar_format_ustar * header);
static uid_t mtar_format_ustar_in_convert_uid(struct mtar_format_ustar * header);
static void mtar_format_ustar_in_free(struct mtar_format_in * f);
static enum mtar_format_in_header_status mtar_format_ustar_in_get_header(struct mtar_format_in * f, struct mtar_format_header * header);
static int mtar_format_ustar_in_last_errno(struct mtar_format_in * f);
static ssize_t mtar_format_ustar_in_prefetch(struct mtar_format_ustar_in * self, ssize_t length);
static ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length);
static ssize_t mtar_format_ustar_in_read_buffer(struct mtar_format_ustar_in * f, void * data, ssize_t length);
static int mtar_format_ustar_in_skip_file(struct mtar_format_in * f);

static struct mtar_format_in_ops mtar_format_ustar_in_ops = {
	.free       = mtar_format_ustar_in_free,
	.get_header = mtar_format_ustar_in_get_header,
	.last_errno = mtar_format_ustar_in_last_errno,
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

	if (self->io)
		self->io->ops->free(self->io);

	if (self->buffer)
		free(self->buffer);
	self->buffer = 0;

	free(f->data);
	free(f);
}

enum mtar_format_in_header_status mtar_format_ustar_in_get_header(struct mtar_format_in * f, struct mtar_format_header * header) {
	struct mtar_format_ustar_in * self = f->data;
	ssize_t nbRead = mtar_format_ustar_in_prefetch(self, 2560);
	if (nbRead == 0)
		return MTAR_FORMAT_HEADER_NOT_FOUND;
	if (nbRead < 0)
		return MTAR_FORMAT_HEADER_BAD_HEADER;

	mtar_format_init_header(header);

	do {
		char buffer[512];
		struct mtar_format_ustar * h = (struct mtar_format_ustar *) buffer;
		nbRead = mtar_format_ustar_in_read_buffer(self, buffer, 512);

		if (h->filename[0] == '\0')
			return MTAR_FORMAT_HEADER_NOT_FOUND;

		// no header found
		if (nbRead < 512)
			return MTAR_FORMAT_HEADER_BAD_HEADER;

		if (mtar_format_ustar_in_check_header(h)) {
			// header checksum failed !!!
			return MTAR_FORMAT_HEADER_BAD_CHECKSUM;
		}

		ssize_t next_read;
		switch (h->flag) {
			case 'L':
				next_read = mtar_format_ustar_in_convert_size(h);
				nbRead = mtar_format_ustar_in_read_buffer(self, buffer, 512 + next_read - next_read % 512);
				strncpy(header->path, buffer, next_read);
				continue;

			case 'K':
				next_read = mtar_format_ustar_in_convert_size(h);
				nbRead = mtar_format_ustar_in_read_buffer(self, buffer, 512 + next_read - next_read % 512);
				strncpy(header->link, buffer, next_read);
				continue;

			case '1':
			case '2':
				if (header->link[0] == 0)
					strncpy(header->link, h->linkname, 256);
				break;

			case '3':
			case '4':
				header->dev = mtar_format_ustar_in_convert_dev(h);
				break;
		}

		if (header->path[0] == 0)
			strncpy(header->path, h->filename, 256);
		header->filename = header->path;
		header->size = mtar_format_ustar_in_convert_size(h);
		sscanf(h->filemode, "%o", &header->mode);
		header->mtime = mtar_format_ustar_in_convert_time(h);
		header->uid = mtar_format_ustar_in_convert_uid(h);
		strcpy(header->uname, h->uname);
		header->gid = mtar_format_ustar_in_convert_gid(h);
		strcpy(header->gname, h->gname);

		switch (h->flag) {
			case '0':
				header->mode |= S_IFREG;
				break;

			case '2':
				header->mode |= S_IFLNK;
				break;

			case '3':
				header->mode |= S_IFCHR;
				break;

			case '4':
				header->mode |= S_IFBLK;
				break;

			case '5':
				header->mode |= S_IFDIR;
				break;

			case '6':
				header->mode |= S_IFIFO;
				break;
		}
	} while (!header->filename);

	self->skipsize = self->filesize = header->size;

	if (header->size > 0 && header->size % 512)
		self->skipsize = 512 + header->size - header->size % 512;

	return MTAR_FORMAT_HEADER_OK;
}

int mtar_format_ustar_in_last_errno(struct mtar_format_in * f) {
	struct mtar_format_ustar_in * self = f->data;
	return self->io->ops->last_errno(self->io);
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
	struct mtar_format_ustar_in * self = f->data;
	if (length > self->filesize)
		length = self->filesize;
	ssize_t nbRead = mtar_format_ustar_in_read_buffer(self, data, length);
	if (nbRead > 0) {
		self->filesize -= nbRead;
		self->skipsize -= nbRead;
	}
	if (self->filesize == 0 && self->skipsize > 0)
		mtar_format_ustar_in_skip_file(f);
	return nbRead;
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

	char * pdata = data;
	ssize_t r = self->io->ops->read(self->io, pdata + nbRead, length - nbRead);
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
	data->buffer = 0;
	data->bufferSize = 0;
	data->bufferUsed = 0;
	data->filesize = 0;
	data->skipsize = 0;

	struct mtar_format_in * self = malloc(sizeof(struct mtar_format_in));
	self->ops = &mtar_format_ustar_in_ops;
	self->data = data;

	return self;
}

int mtar_format_ustar_in_skip_file(struct mtar_format_in * f) {
	struct mtar_format_ustar_in * self = f->data;
	if (self->skipsize == 0)
		return 0;
	if (self->bufferUsed > 0) {
		if (self->skipsize < self->bufferUsed) {
			memmove(self->buffer, self->buffer + self->skipsize, self->bufferUsed - self->skipsize);
			self->bufferUsed -= self->skipsize;
			self->filesize = 0;
			self->skipsize = 0;
			return 0;
		} else {
			self->filesize -= self->bufferUsed;
			self->skipsize -= self->bufferUsed;
			self->bufferUsed = 0;
		}
	}
	if (self->skipsize > 0) {
		off_t next_pos = self->io->ops->pos(self->io) + self->skipsize;
		off_t new_pos = self->io->ops->forward(self->io, self->skipsize);
		if (new_pos == (off_t) -1)
			return 1;
		self->filesize = self->skipsize = 0;
		return new_pos != next_pos;
	}
	return 0;
}

