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
*  Last modified: Mon, 13 Jun 2011 15:43:01 +0200                       *
\***********************************************************************/

// free, malloc, realloc
#include <stdlib.h>

#include "common.h"

struct mtar_format_ustar_in {
	struct mtar_io_in * io;
	off_t position;
	ssize_t size;
};


static void mtar_format_ustar_in_free(struct mtar_format_in * f);
static int mtar_format_ustar_in_getHeader(struct mtar_format_in * f, struct mtar_format_header * header);
static ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length);

static struct mtar_format_in_ops mtar_format_ustar_in_ops = {
	.free      = mtar_format_ustar_in_free,
	.getHeader = mtar_format_ustar_in_getHeader,
	.read      = mtar_format_ustar_in_read,
};


void mtar_format_ustar_in_free(struct mtar_format_in * f) {
	free(f->data);
	free(f);
}

int mtar_format_ustar_in_getHeader(struct mtar_format_in * f, struct mtar_format_header * header) {
	return 0;
}

ssize_t mtar_format_ustar_in_read(struct mtar_format_in * f, void * data, ssize_t length) {
	return 0;
}

struct mtar_format_in * mtar_format_ustar_newIn(struct mtar_io_in * io, const struct mtar_option * option __attribute__((unused))) {
	if (!io)
		return 0;

	struct mtar_format_ustar_in * data = malloc(sizeof(struct mtar_format_ustar_in));
	data->io = io;
	data->position = 0;
	data->size = 0;

	struct mtar_format_in * self = malloc(sizeof(struct mtar_format_in));
	self->ops = &mtar_format_ustar_in_ops;
	self->data = data;

	return self;
}

