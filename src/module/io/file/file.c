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
*  Last modified: Tue, 03 May 2011 16:35:59 +0200                       *
\***********************************************************************/

// malloc
#include <stdlib.h>

#include <mtar/option.h>

#include "common.h"

static struct mtar_io_ops file_ops = {
	.canSeek = mtar_io_file_can_seek,
	.close   = mtar_io_file_close,
	.free    = mtar_io_file_free,
	.pos     = mtar_io_file_pos,
	.read    = mtar_io_file_read,
	.seek    = mtar_io_file_seek,
	.write   = mtar_io_file_write,
};

static struct mtar_io * mtar_io_file(int fd, int flags, const struct mtar_option * option);


struct mtar_io * mtar_io_file(int fd, int flags __attribute__((unused)), const struct mtar_option * option __attribute__((unused))) {
	struct mtar_io_file * data = malloc(sizeof(struct mtar_io_file));
	data->fd = fd;
	data->pos = 0;

	struct mtar_io * io = malloc(sizeof(struct mtar_io));
	io->ops = &file_ops;
	io->data = data;

	return io;
}

__attribute__((constructor))
static void mtar_io_file_init() {
	mtar_io_register("file", mtar_io_file);
}

