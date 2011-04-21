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
*  Last modified: Thu, 21 Apr 2011 22:56:57 +0200                       *
\***********************************************************************/

// malloc
#include <stdlib.h>

#include "common.h"


static struct mtar_format * mtar_format_ustar(struct mtar_io * io, struct mtar_option * option);

static const char * format_ustar_name = "ustar";


__attribute__((constructor))
static void format_init() {
	mtar_format_register(format_ustar_name, mtar_format_ustar);
}


struct mtar_format * mtar_format_ustar(struct mtar_io * io, struct mtar_option * option __attribute__((unused))) {
	if (!io)
		return 0;

	struct mtar_format_ustar * data = malloc(sizeof(struct mtar_format_ustar));
	data->io = io;
	data->pos = 0;

	struct mtar_format * self = malloc(sizeof(struct mtar_format));
	self->format = format_ustar_name;
	self->ops = 0;
	self->data = data;

	return self;
}

