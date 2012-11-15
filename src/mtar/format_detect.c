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
*  Last modified: Thu, 15 Nov 2012 13:38:17 +0100                           *
\***************************************************************************/

// free, malloc
#include <stdlib.h>
// memcpy
#include <string.h>

#include "format.h"
#include "io.h"

struct mtar_format_detect_private {
	struct mtar_io_reader * reader;

	char * buffer;
	char * buffer_pos;
	ssize_t buffer_size;
};

static ssize_t mtar_format_detect_block_size(struct mtar_io_reader * io);
static int mtar_format_detect_close(struct mtar_io_reader * io);
static off_t mtar_format_detect_forward(struct mtar_io_reader * io, off_t offset);
static void mtar_format_detect_free(struct mtar_io_reader * io);
static int mtar_format_detect_last_errno(struct mtar_io_reader * io);
static off_t mtar_format_detect_position(struct mtar_io_reader * io);
static ssize_t mtar_format_detect_read(struct mtar_io_reader * io, void * data, ssize_t length);

struct mtar_io_reader_ops mtar_format_detect_ops = {
	.block_size = mtar_format_detect_block_size,
	.close      = mtar_format_detect_close,
	.forward    = mtar_format_detect_forward,
	.free       = mtar_format_detect_free,
	.last_errno = mtar_format_detect_last_errno,
	.position   = mtar_format_detect_position,
	.read       = mtar_format_detect_read,
};


struct mtar_format_reader * mtar_format_auto_detect(struct mtar_io_reader * reader, const struct mtar_option * option) {
	if (reader == NULL)
		return NULL;

	ssize_t buffer_size = reader->ops->block_size(reader);
	char * buffer = malloc(buffer_size);

	ssize_t nb_read = reader->ops->read(reader, buffer, buffer_size);
	if (nb_read < 0) {
		free(buffer);
		return NULL;
	}

	struct mtar_format * format = mtar_format_auto_detect2(buffer, nb_read);
	if (format == NULL) {
		free(buffer);
		return NULL;
	}

	struct mtar_format_detect_private * self = malloc(sizeof(struct mtar_format_detect_private));
	self->reader = reader;
	self->buffer = self->buffer_pos = buffer;
	self->buffer_size = nb_read;

	struct mtar_io_reader * io_reader = malloc(sizeof(struct mtar_io_reader));
	io_reader->ops = &mtar_format_detect_ops;
	io_reader->data = self;

	return format->new_reader(io_reader, option);
}

static ssize_t mtar_format_detect_block_size(struct mtar_io_reader * io) {
	struct mtar_format_detect_private * self = io->data;
	return self->reader->ops->block_size(self->reader);
}

static int mtar_format_detect_close(struct mtar_io_reader * io) {
	struct mtar_format_detect_private * self = io->data;
	return self->reader->ops->close(self->reader);
}

static off_t mtar_format_detect_forward(struct mtar_io_reader * io, off_t offset) {
	struct mtar_format_detect_private * self = io->data;

	if (self->buffer != NULL) {
		ssize_t buffer_available = self->buffer_size + self->buffer - self->buffer_pos;

		if (offset >= buffer_available) {
			offset -= buffer_available;

			free(self->buffer);
			self->buffer = self->buffer_pos = NULL;
		} else {
			self->buffer_pos += offset;
			return self->buffer_pos - self->buffer;
		}
	}

	return self->reader->ops->forward(self->reader, offset);
}

static void mtar_format_detect_free(struct mtar_io_reader * io) {
	struct mtar_format_detect_private * self = io->data;
	self->reader->ops->free(self->reader);
	free(self->buffer);
	self->buffer = self->buffer_pos = NULL;
}

static int mtar_format_detect_last_errno(struct mtar_io_reader * io) {
	struct mtar_format_detect_private * self = io->data;
	return self->reader->ops->last_errno(self->reader);
}

static off_t mtar_format_detect_position(struct mtar_io_reader * io) {
	struct mtar_format_detect_private * self = io->data;

	if (self->buffer != NULL)
		return self->buffer_pos - self->buffer;

	return self->reader->ops->position(self->reader);
}

static ssize_t mtar_format_detect_read(struct mtar_io_reader * io, void * data, ssize_t length) {
	struct mtar_format_detect_private * self = io->data;

	if (self->buffer != NULL) {
		ssize_t buffer_available = self->buffer_size + self->buffer - self->buffer_pos;

		if (length == buffer_available) {
			memcpy(data, self->buffer_pos, buffer_available);
			free(self->buffer);
			self->buffer = self->buffer_pos = NULL;
			return length;
		} else if (length >= buffer_available) {
			char * cbuffer = data;
			ssize_t nb_read = self->reader->ops->read(self->reader, cbuffer + buffer_available, length - buffer_available);
			if (nb_read < 0)
				return nb_read;

			memcpy(data, self->buffer_pos, buffer_available);

			free(self->buffer);
			self->buffer = self->buffer_pos = NULL;

			return nb_read + buffer_available;
		} else {
			memcpy(data, self->buffer_pos, length);

			return self->buffer_pos - self->buffer;
		}
	}

	return self->reader->ops->read(self->reader, data, length);
}

