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
*  Last modified: Fri, 26 Oct 2012 19:29:12 +0200                           *
\***************************************************************************/

#define _GNU_SOURCE
// asprintf
#include <stdio.h>
// free, malloc
#include <stdlib.h>
// strdup
#include <string.h>
// S_IFDIR
#include <sys/stat.h>
// mktime
#include <time.h>

#include <mtar/hashtable.h>
#include <mtar/io.h>
#include <mtar/util.h>

#include "mtf.h"

struct mtar_format_mtf_reader {
	struct mtar_io_reader * io;
	off_t position;

	char * buffer;
	ssize_t buffer_size;

	ssize_t file_size;
	ssize_t skip_size;

	struct mtar_hashtable * directories;
	uint32_t last_dir_id;
};

static void mtar_format_mtf_reader_free(struct mtar_format_reader * f);
static enum mtar_format_reader_header_status mtar_format_mtf_reader_get_header(struct mtar_format_reader * f, struct mtar_format_header * header);
static char * mtar_format_mtf_reader_get_path(const void * string, size_t length, enum mtar_format_mtf_string_type type);
static char * mtar_format_mtf_reader_get_string(const void * string, size_t length, enum mtar_format_mtf_string_type type);
static time_t mtar_format_mtf_reader_get_time(const unsigned char date[5]);
static int mtar_format_mtf_reader_last_errno(struct mtar_format_reader * f);
static void mtar_format_mtf_reader_next_volume(struct mtar_format_reader * f, struct mtar_io_reader * next_volume);
static ssize_t mtar_format_mtf_reader_read(struct mtar_format_reader * f, void * data, ssize_t length);
static enum mtar_format_reader_header_status mtar_format_mtf_reader_skip_file(struct mtar_format_reader * f);

static struct mtar_format_reader_ops mtar_format_mtf_reader_ops = {
	.free        = mtar_format_mtf_reader_free,
	.get_header  = mtar_format_mtf_reader_get_header,
	.last_errno  = mtar_format_mtf_reader_last_errno,
	.next_volume = mtar_format_mtf_reader_next_volume,
	.read        = mtar_format_mtf_reader_read,
	.skip_file   = mtar_format_mtf_reader_skip_file,
};


static void mtar_format_mtf_reader_free(struct mtar_format_reader * f) {
	struct mtar_format_mtf_reader * self = f->data;

	if (self->io != NULL)
		self->io->ops->free(self->io);

	free(self->buffer);
	self->buffer = NULL;

	mtar_hashtable_free(self->directories);
	self->directories = NULL;

	free(f->data);
	free(f);
}

static enum mtar_format_reader_header_status mtar_format_mtf_reader_get_header(struct mtar_format_reader * f, struct mtar_format_header * header) {
	struct mtar_format_mtf_reader * self = f->data;

	int i;
	for (i = 0; ; i++) {
		char buffer[1024];
		ssize_t nb_read = self->io->ops->read(self->io, buffer, 1024);
		if (nb_read < 0)
			return mtar_format_header_error;
		if (nb_read == 0)
			return mtar_format_header_not_found;
		if (nb_read < 1024)
			return mtar_format_header_bad_header;

		struct mtar_format_mtf_descriptor_block * block = (struct mtar_format_mtf_descriptor_block *) buffer;
		struct mtar_format_mtf_dirb * dirb = NULL;
		struct mtar_format_mtf_file * file = NULL;
		struct mtar_format_mtf_stream * strm = NULL;

		char * dir_id = NULL;
		char * dir_name = NULL;
		char * path = NULL;

		uint32_t stream_offset;

		switch (block->type) {
			case mtar_format_mtf_descriptor_block_dirb:
				dirb = (struct mtar_format_mtf_dirb *) (buffer + 52);

				self->last_dir_id = dirb->directory_id;

				asprintf(&dir_id, "dir_%u", dirb->directory_id);
				path = mtar_format_mtf_reader_get_path(buffer + dirb->directory_name.offset, dirb->directory_name.size, block->string_type);
				mtar_hashtable_put(self->directories, dir_id, path);

				mtar_format_init_header(header);
				header->path = strdup(path);
				header->mode = 0644 | S_IFDIR;
				header->mtime = mtar_format_mtf_reader_get_time(dirb->last_modification_time);
				// header->uid
				// header->uname
				// header->gid
				// header->gname

				self->skip_size = self->file_size = 0;

				return mtar_format_header_ok;

			case mtar_format_mtf_descriptor_block_file:
				file = (struct mtar_format_mtf_file *) (buffer + 52);
				stream_offset = block->offset_to_first_event;
				strm = (struct mtar_format_mtf_stream *) (buffer + stream_offset);

				asprintf(&dir_id, "dir_%u", file->directory_id);
				dir_name = mtar_hashtable_value(self->directories, dir_id);

				path = mtar_format_mtf_reader_get_path(buffer + file->file_name.offset, file->file_name.size, block->string_type);

				while (!(strm->type == mtar_format_mtf_stream_stan || strm->type == mtar_format_mtf_stream_spad)) {
					stream_offset += strm->stream_length + 26 - (strm->stream_length + 22) % 4;
					strm = (struct mtar_format_mtf_stream *) (buffer + stream_offset);
				}

				stream_offset += 22;

				mtar_format_init_header(header);
				asprintf(&header->path, "%s/%s", dir_name, path);
				header->size = stream_offset < 1024 ? strm->stream_length : 0;
				header->mode = 0644;
				header->mtime = mtar_format_mtf_reader_get_time(file->last_modification_time);
				// header->uid
				// header->uname
				// header->gid
				// header->gname

				free(dir_id);
				free(path);

				self->skip_size = self->file_size = header->size;

				if (self->skip_size > 0) {
					self->skip_size -= 1024 - stream_offset;
					if (self->skip_size > 0 && self->skip_size % 1024)
						self->skip_size = 1024 + self->skip_size - self->skip_size % 1024;
				}

				return mtar_format_header_ok;

			default:
				continue;
		}
	}
}

static char * mtar_format_mtf_reader_get_path(const void * string, size_t length, enum mtar_format_mtf_string_type type) {
	char * result = NULL;
	const char * c_string = NULL;
	const unsigned short * uni_string = NULL;
	size_t i, uni_length, offset;

	switch (type) {
		case mtar_format_mtf_string_type_no_string:
		default:
			return result;

		case mtar_format_mtf_string_type_ansi_string:
			result = malloc(length + 1);
			c_string = string;

			for (i = 0; i < length; i++)
				result[i] = c_string[i] == '\0' ? '/' : c_string[i];
			result[i] = '\0';

			if (length == 1 && result[0] == '/')
				result[0] = '.';
			else if (result[length - 1] == '/')
				result[length - 1] = '\0';

			return result;

		case mtar_format_mtf_string_type_unicode_string:
			length >>= 1;
			uni_string = string;

			uni_length = 0;
			for (i = 0; i < length; i++) {
				uint32_t unicode;
				if (((uni_string[i] & 0xD8) == 0xD8) && (i + 1 < length) && ((uni_string[i + 1] & 0xDB) == 0xDB)) {
					uint32_t w = (uni_string[i] & 0x3B0) >> 6;
					unicode = ((w + 1) << 16) + ((uni_string[i] & 0x3F00) >> 14) + ((uni_string[i + 1] & 0xFF00) >> 6) + (uni_string[i + 1] & 0x3);
				} else
					unicode = uni_string[i];

				if (unicode < 0x7F)
					uni_length++;
				else if (unicode < 0x800)
					uni_length += 2;
				else if (unicode < 0x10000)
					uni_length += 3;
				else
					uni_length += 4;
			}

			result = malloc(uni_length + 1);

			for (i = 0, offset = 0; i < length; i++) {
				uint32_t unicode;
				if (((uni_string[i] & 0xD8) == 0xD8) && (i + 1 < length) && ((uni_string[i + 1] & 0xDB) == 0xDB)) {
					uint32_t w = (uni_string[i] & 0x3B0) >> 6;
					unicode = ((w + 1) << 16) + ((uni_string[i] & 0x3F00) >> 14) + ((uni_string[i + 1] & 0xFF00) >> 6) + (uni_string[i + 1] & 0x3);
				} else
					unicode = uni_string[i];

				if (unicode == 0)
					unicode = '/';

				if (unicode < 0x7F) {
					result[offset] = unicode;
					offset++;
				} else if (unicode < 0x800) {
					result[offset] = ((unicode & 0x7C0) >> 6) | 0xC0;
					result[offset + 1] = (unicode & 0x3F) | 0x80;
					offset += 2;
				} else if (unicode < 0x10000) {
					result[offset] = ((unicode & 0xF000) >> 12) | 0xE0;
					result[offset + 1] = ((unicode & 0xFC0) >> 6) | 0x80;
					result[offset + 2] = (unicode & 0x3F) | 0x80;
					offset += 3;
				} else {
					result[offset] = ((unicode & 0x1C0000) >> 18) | 0xF0;
					result[offset + 1] = ((unicode & 0x3F000) >> 12) | 0x80;
					result[offset + 2] = ((unicode & 0xFC0) >> 6) | 0x80;
					result[offset + 3] = (unicode & 0x3F) | 0x80;
					offset += 4;
				}
			}

			result[i] = '\0';

			if (length == 1 && result[0] == '/')
				result[0] = '.';
			else if (result[length - 1] == '/')
				result[length - 1] = '\0';

			return result;
	}
}

static char * mtar_format_mtf_reader_get_string(const void * string, size_t length, enum mtar_format_mtf_string_type type) {
	char * result = NULL;
	const char * c_string = NULL;
	const unsigned short * uni_string = NULL;
	size_t i, uni_length, offset;

	switch (type) {
		case mtar_format_mtf_string_type_no_string:
		default:
			return result;

		case mtar_format_mtf_string_type_ansi_string:
			result = malloc(length + 1);
			c_string = string;

			for (i = 0; i < length; i++)
				result[i] = c_string[i];
			result[i] = '\0';

			return result;

		case mtar_format_mtf_string_type_unicode_string:
			length >>= 1;
			uni_string = string;

			uni_length = 0;
			for (i = 0; i < length; i++) {
				uint32_t unicode;
				if (((uni_string[i] & 0xD8) == 0xD8) && (i + 1 < length) && ((uni_string[i + 1] & 0xDB) == 0xDB)) {
					uint32_t w = (uni_string[i] & 0x3B0) >> 6;
					unicode = ((w + 1) << 16) + ((uni_string[i] & 0x3F00) >> 14) + ((uni_string[i + 1] & 0xFF00) >> 6) + (uni_string[i + 1] & 0x3);
				} else
					unicode = uni_string[i];

				if (unicode < 0x7F)
					uni_length++;
				else if (unicode < 0x800)
					uni_length += 2;
				else if (unicode < 0x10000)
					uni_length += 3;
				else
					uni_length += 4;
			}

			result = malloc(uni_length + 1);

			for (i = 0, offset = 0; i < length; i++) {
				uint32_t unicode;
				if (((uni_string[i] & 0xD8) == 0xD8) && (i + 1 < length) && ((uni_string[i + 1] & 0xDB) == 0xDB)) {
					uint32_t w = (uni_string[i] & 0x3B0) >> 6;
					unicode = ((w + 1) << 16) + ((uni_string[i] & 0x3F00) >> 14) + ((uni_string[i + 1] & 0xFF00) >> 6) + (uni_string[i + 1] & 0x3);
				} else
					unicode = uni_string[i];

				if (unicode < 0x7F) {
					result[offset] = unicode;
					offset++;
				} else if (unicode < 0x800) {
					result[offset] = ((unicode & 0x7C0) >> 6) | 0xC0;
					result[offset + 1] = (unicode & 0x3F) | 0x80;
					offset += 2;
				} else if (unicode < 0x10000) {
					result[offset] = ((unicode & 0xF000) >> 12) | 0xE0;
					result[offset + 1] = ((unicode & 0xFC0) >> 6) | 0x80;
					result[offset + 2] = (unicode & 0x3F) | 0x80;
					offset += 3;
				} else {
					result[offset] = ((unicode & 0x1C0000) >> 18) | 0xF0;
					result[offset + 1] = ((unicode & 0x3F000) >> 12) | 0x80;
					result[offset + 2] = ((unicode & 0xFC0) >> 6) | 0x80;
					result[offset + 3] = (unicode & 0x3F) | 0x80;
					offset += 4;
				}
			}

			result[i] = '\0';

			if (length == 1 && result[0] == '/')
				result[0] = '.';
			else if (result[length - 1] == '/')
				result[length - 1] = '\0';

			return result;
	}
}

static time_t mtar_format_mtf_reader_get_time(const unsigned char date[5]) {
	struct tm time = {
		.tm_sec  = date[4] & 0x3F,
		.tm_min  = (date[3] & 0x0F) << 2 | date[4] >> 6,
		.tm_hour = (date[2] & 0x01) << 4 | date[3] >> 4,
		.tm_mday = (date[2] & 0x3E) >> 1,
		.tm_mon  = (date[1] & 0x03) | (date[2] & 0xC0) >> 6,
		.tm_year = ((date[0] << 6) | (date[1] >> 2)) - 1900,
	};
	return mktime(&time);
}

static int mtar_format_mtf_reader_last_errno(struct mtar_format_reader * f) {
	struct mtar_format_mtf_reader * self = f->data;
	return self->io->ops->last_errno(self->io);
}

static void mtar_format_mtf_reader_next_volume(struct mtar_format_reader * f, struct mtar_io_reader * next_volume) {
}

static ssize_t mtar_format_mtf_reader_read(struct mtar_format_reader * f, void * data, ssize_t length) {
}

static enum mtar_format_reader_header_status mtar_format_mtf_reader_skip_file(struct mtar_format_reader * f) {
	struct mtar_format_mtf_reader * self = f->data;

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

struct mtar_format_reader * mtar_format_mtf_new_reader(struct mtar_io_reader * io, const struct mtar_option * option __attribute__((unused))) {
	if (io == NULL)
		return NULL;

	struct mtar_format_mtf_reader * self = malloc(sizeof(struct mtar_format_mtf_reader));
	self->io = io;
	self->position = 0;

	self->buffer = malloc(1024);
	self->buffer_size = 1024;

	self->file_size = 0;
	self->skip_size = 0;

	self->directories = mtar_hashtable_new2(mtar_util_compute_hash_string, mtar_util_basic_free);
	self->last_dir_id = 0;

	struct mtar_format_reader * reader = malloc(sizeof(struct mtar_format_reader));
	reader->ops = &mtar_format_mtf_reader_ops;
	reader->data = self;

	return reader;
}

