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
*  Last modified: Thu, 26 May 2011 13:07:47 +0200                       *
\***********************************************************************/

// malloc
#include <stdlib.h>

#include "common.h"


static struct mtar_format * mtar_format_ustar(struct mtar_io * io, const struct mtar_option * option);

static const char * format_ustar_name = "ustar";
static struct mtar_format_ops format_ustar_ops = {
	.addFile   = mtar_format_ustar_addFile,
	.addLink   = mtar_format_ustar_addLink,
	.endOfFile = mtar_format_ustar_endOfFile,
	.free      = mtar_format_ustar_free,
	.getHeader = mtar_format_ustar_getHeader,
	.write     = mtar_format_ustar_write,
};


__attribute__((constructor))
static void format_init() {
	mtar_format_register(format_ustar_name, mtar_format_ustar);
}


struct mtar_format * mtar_format_ustar(struct mtar_io * io, const struct mtar_option * option __attribute__((unused))) {
	if (!io)
		return 0;

	struct mtar_format_ustar * data = malloc(sizeof(struct mtar_format_ustar));
	data->io = io;
	data->position = 0;
	data->size = 0;

	struct mtar_format * self = malloc(sizeof(struct mtar_format));
	self->format = format_ustar_name;
	self->ops = &format_ustar_ops;
	self->data = data;

	return self;
}

