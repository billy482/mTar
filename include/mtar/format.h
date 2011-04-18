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
*  Last modified: Mon, 18 Apr 2011 23:28:55 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FORMAT_H__
#define __MTAR_FORMAT_H__

// ssize_t
#include <sys/types.h>

#include "option.h"

struct mtar_io;
struct mtar_option;

struct mtar_format_header {
	const char path[256];
	const char * filename;
	ssize_t size;
};

struct mtar_format {
	mtar_format_enum format;
	struct mtar_format_ops {
		int (*addFile)(struct mtar_format * f, const char * filename);
		int (*endOfFile)(struct mtar_format * f);
		void (*free)(struct mtar_format * f);
		int (*getHeader)(struct mtar_format * f, struct mtar_format_header * header);
		ssize_t (*read)(struct mtar_format * f, void * data, ssize_t length);
		ssize_t (*write)(struct mtar_format * f, const void * data, ssize_t length);
	} * ops;
	void * data;
};

typedef struct mtar_format * (*mtar_format_f)(struct mtar_io * io, struct mtar_option * option, struct mtar_verbose * verbose);

struct mtar_format * mtar_format_get(struct mtar_io * io, struct mtar_option * option, struct mtar_verbose * verbose);
void mtar_format_register(const char * name, mtar_format_f format);

#endif

