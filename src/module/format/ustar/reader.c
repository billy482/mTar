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
*  Last modified: Thu, 15 Nov 2012 14:21:21 +0100                           *
\***************************************************************************/

// bool
#include <stdbool.h>
// sscanf, snprintf
#include <stdio.h>
// free, malloc, realloc
#include <stdlib.h>
// memcpy, memmove, memset, strncpy, strncmp
#include <string.h>
// S_*
#include <sys/stat.h>

#include <mtar/io.h>

#include "ustar.h"

struct mtar_format_ustar_reader {
	struct mtar_io_reader * io;
	off_t position;

	char * buffer;
	ssize_t buffer_size;

	ssize_t file_size;
	ssize_t skip_size;
};


static bool mtar_format_ustar_reader_check_header(struct mtar_format_ustar * header);
static dev_t mtar_format_ustar_reader_convert_dev(struct mtar_format_ustar * header);
static gid_t mtar_format_ustar_reader_convert_gid(struct mtar_format_ustar * header);
static ssize_t mtar_format_ustar_reader_convert_size(const char * size);
static time_t mtar_format_ustar_reader_convert_time(struct mtar_format_ustar * header);
static uid_t mtar_format_ustar_reader_convert_uid(struct mtar_format_ustar * header);
static void mtar_format_ustar_reader_free(struct mtar_format_reader * f);
static enum mtar_format_reader_header_status mtar_format_ustar_reader_get_header(struct mtar_format_reader * f, struct mtar_format_header * header);
static int mtar_format_ustar_reader_last_errno(struct mtar_format_reader * f);
static void mtar_format_ustar_reader_next_volume(struct mtar_format_reader * f, struct mtar_io_reader * next_volume);
static ssize_t mtar_format_ustar_reader_read(struct mtar_format_reader * f, void * data, ssize_t length);
static enum mtar_format_reader_header_status mtar_format_ustar_reader_skip_file(struct mtar_format_reader * f);

static struct mtar_format_reader_ops mtar_format_ustar_reader_ops = {
	.free        = mtar_format_ustar_reader_free,
	.get_header  = mtar_format_ustar_reader_get_header,
	.last_errno  = mtar_format_ustar_reader_last_errno,
	.next_volume = mtar_format_ustar_reader_next_volume,
	.read        = mtar_format_ustar_reader_read,
	.skip_file   = mtar_format_ustar_reader_skip_file,
};


bool mtar_format_ustar_auto_detect(const void * buffer, ssize_t length) {
	if (length < 512)
		return false;

	const unsigned char * ptr = buffer;
	unsigned int i, sum = 0;
	for (i = 0; i < 512; i++)
		sum += ptr[i];

	const struct mtar_format_ustar * raw_header = buffer;
	ptr = (const unsigned char *) &raw_header->checksum;
	for (i = 0; i < 8; i++)
		sum += ' ' - ptr[i];

	unsigned int sum2;
	sscanf(raw_header->checksum, "%06o", &sum2);

	return sum == sum2;
}

static bool mtar_format_ustar_reader_check_header(struct mtar_format_ustar * header) {
	char checksum[8];
	strncpy(checksum, header->checksum, 8);
	memset(header->checksum, ' ', 8);

	unsigned char * ptr = (unsigned char *) header;
	unsigned int i, sum = 0;
	for (i = 0; i < 512; i++)
		sum += ptr[i];
	snprintf(header->checksum, 7, "%06o", sum);

	return !strncmp(checksum, header->checksum, 8);
}

static dev_t mtar_format_ustar_reader_convert_dev(struct mtar_format_ustar * header) {
	unsigned int major = 0, minor = 0;

	sscanf(header->devmajor, "%o", &major);
	sscanf(header->devminor, "%o", &minor);

	return (major << 8 ) | minor;
}

static gid_t mtar_format_ustar_reader_convert_gid(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->gid, "%o", &result);
	return result;
}

static ssize_t mtar_format_ustar_reader_convert_size(const char * size) {
	const unsigned char * usize = (const unsigned char *) size;
	if (usize[0] == 0x80) {
		short i;
		ssize_t result = 0;
		for (i = 1; i < 12; i++) {
			result <<= 8;
			result |= size[i];
		}
		return result;
	} else {
		unsigned long long result = 0;
		sscanf(size, "%llo", &result);
		return result;
	}
}

static time_t mtar_format_ustar_reader_convert_time(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->mtime, "%o", &result);
	return result;
}

static uid_t mtar_format_ustar_reader_convert_uid(struct mtar_format_ustar * header) {
	unsigned int result;
	sscanf(header->uid, "%o", &result);
	return result;
}

static void mtar_format_ustar_reader_free(struct mtar_format_reader * f) {
	struct mtar_format_ustar_reader * self = f->data;

	if (self->io != NULL)
		self->io->ops->free(self->io);

	free(self->buffer);
	self->buffer = NULL;

	free(f->data);
	free(f);
}

static enum mtar_format_reader_header_status mtar_format_ustar_reader_get_header(struct mtar_format_reader * f, struct mtar_format_header * header) {
	struct mtar_format_ustar_reader * self = f->data;

	ssize_t nb_read = self->io->ops->read(self->io, self->buffer, 512);
	if (nb_read == 0)
		return mtar_format_header_not_found;
	if (nb_read < 0)
		return mtar_format_header_bad_header;

	struct mtar_format_ustar * raw_header = (struct mtar_format_ustar *) self->buffer;

	if (raw_header->filename[0] == '\0')
		return mtar_format_header_not_found;

	if (!mtar_format_ustar_reader_check_header(raw_header))
		return mtar_format_header_bad_checksum;

	mtar_format_init_header(header);

	for (;;) {
		ssize_t next_read;
		switch (raw_header->flag) {
			case 'L':
				next_read = 512 + mtar_format_ustar_reader_convert_size(raw_header->size);
				if (next_read % 512)
					next_read -= next_read % 512;

				if (next_read > self->buffer_size)
					self->buffer = realloc(self->buffer, next_read);

				nb_read = self->io->ops->read(self->io, self->buffer, next_read);
				if (nb_read < next_read) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_header;
				}
				header->path = strdup(self->buffer);

				nb_read = self->io->ops->read(self->io, self->buffer, 512);
				if (nb_read < 512) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_header;
				}
				if (!mtar_format_ustar_reader_check_header(raw_header)) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_checksum;
				}
				continue;

			case 'K':
				next_read = 512 + mtar_format_ustar_reader_convert_size(raw_header->size);
				if (next_read % 512)
					next_read -= next_read % 512;

				if (next_read > self->buffer_size)
					self->buffer = realloc(self->buffer, next_read);

				nb_read = self->io->ops->read(self->io, self->buffer, next_read);
				if (nb_read < next_read) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_header;
				}
				header->link = strdup(self->buffer);

				nb_read = self->io->ops->read(self->io, self->buffer, 512);
				if (nb_read < 512) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_header;
				}
				if (!mtar_format_ustar_reader_check_header(raw_header)) {
					mtar_format_free_header(header);
					return mtar_format_header_bad_checksum;
				}
				continue;

			case 'M':
				header->position = mtar_format_ustar_reader_convert_size(raw_header->position);
				break;

			case '1':
			case '2':
				if (!header->link) {
					if (strlen(raw_header->linkname) > 100) {
						header->link = malloc(101);
						strncpy(header->link, raw_header->linkname, 100);
						header->link[100] = '\0';
					} else {
						header->link = strdup(raw_header->linkname);
					}
				}
				break;

			case '3':
			case '4':
				header->dev = mtar_format_ustar_reader_convert_dev(raw_header);
				break;

			case 'V':
				header->is_label = true;
				break;
		}

		if (header->path == NULL) {
			if (strlen(raw_header->filename) > 100) {
				header->path = malloc(101);
				strncpy(header->path, raw_header->filename, 100);
				header->path[100] = '\0';
			} else {
				header->path = strdup(raw_header->filename);
			}
		}
		header->size = mtar_format_ustar_reader_convert_size(raw_header->size);
		sscanf(raw_header->filemode, "%o", &header->mode);
		header->mtime = mtar_format_ustar_reader_convert_time(raw_header);
		header->uid = mtar_format_ustar_reader_convert_uid(raw_header);
		strcpy(header->uname, raw_header->uname);
		header->gid = mtar_format_ustar_reader_convert_gid(raw_header);
		strcpy(header->gname, raw_header->gname);

		switch (raw_header->flag) {
			case '0':
			case '1':
			case 'M':
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

		break;
	}

	self->skip_size = self->file_size = header->size;

	if (header->size > 0 && header->size % 512)
		self->skip_size = 512 + header->size - header->size % 512;

	return mtar_format_header_ok;
}

static int mtar_format_ustar_reader_last_errno(struct mtar_format_reader * f) {
	struct mtar_format_ustar_reader * self = f->data;
	return self->io->ops->last_errno(self->io);
}

static void mtar_format_ustar_reader_next_volume(struct mtar_format_reader * f, struct mtar_io_reader * next_volume) {
	struct mtar_format_ustar_reader * self = f->data;

	if (self->io != NULL)
		self->io->ops->free(self->io);

	self->io = next_volume;
}

static ssize_t mtar_format_ustar_reader_read(struct mtar_format_reader * f, void * data, ssize_t length) {
	if (f == NULL || data == NULL)
		return -1;

	struct mtar_format_ustar_reader * self = f->data;

	if (self->file_size == 0)
		return 0;

	if (length > self->file_size)
		length = self->file_size;

	ssize_t nb_read = self->io->ops->read(self->io, data, length);

	if (nb_read > 0) {
		self->file_size -= nb_read;
		self->skip_size -= nb_read;
	}

	if (self->file_size == 0 && self->skip_size > 0)
		mtar_format_ustar_reader_skip_file(f);

	return nb_read;
}

static enum mtar_format_reader_header_status mtar_format_ustar_reader_skip_file(struct mtar_format_reader * f) {
	struct mtar_format_ustar_reader * self = f->data;

	if (self->skip_size == 0)
		return mtar_format_header_ok;

	if (self->skip_size > 0) {
		off_t next_pos = self->io->ops->position(self->io) + self->skip_size;
		off_t new_pos = self->io->ops->forward(self->io, self->skip_size);
		if (new_pos == (off_t) -1)
			return mtar_format_header_error;

		if (next_pos > new_pos)
			return mtar_format_header_end_of_tape;

		self->position += self->skip_size;
		self->file_size = self->skip_size = 0;
	}

	return mtar_format_header_ok;
}

struct mtar_format_reader * mtar_format_ustar_new_reader(struct mtar_io_reader * io, const struct mtar_option * option __attribute__((unused))) {
	if (io == NULL)
		return NULL;

	struct mtar_format_ustar_reader * self = malloc(sizeof(struct mtar_format_ustar_reader));
	self->io = io;
	self->position = 0;

	self->buffer = malloc(2560);
	self->buffer_size = 2560;

	self->file_size = 0;
	self->skip_size = 0;

	struct mtar_format_reader * f = malloc(sizeof(struct mtar_format_reader));
	f->ops = &mtar_format_ustar_reader_ops;
	f->data = self;

	return f;
}

