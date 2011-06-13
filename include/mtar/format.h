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
*  Last modified: Fri, 10 Jun 2011 18:51:23 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FORMAT_H__
#define __MTAR_FORMAT_H__

// ssize_t
#include <sys/types.h>

#include "option.h"

struct mtar_io_in;
struct mtar_io_out;
struct mtar_option;

struct mtar_format_header {
	const char path[256];
	const char * filename;
	ssize_t size;
};

struct mtar_format_in {
	const char * format;
	struct mtar_format_in_ops {
		void (*free)(struct mtar_format_in * f);
		int (*getHeader)(struct mtar_format_in * f, struct mtar_format_header * header);
		ssize_t (*read)(struct mtar_format_in * f, void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_format_out {
	const char * format;
	struct mtar_format_out_ops {
		int (*addFile)(struct mtar_format_out * f, const char * filename);
		int (*addLabel)(struct mtar_format_out * f, const char * label);
		int (*addLink)(struct mtar_format_out * f, const char * src, const char * target);
		int (*endOfFile)(struct mtar_format_out * f);
		void (*free)(struct mtar_format_out * f);
		ssize_t (*write)(struct mtar_format_out * f, const void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_format {
	const char * name;
	struct mtar_format_in * (*newIn)(struct mtar_io_in * io, const struct mtar_option * option);
	struct mtar_format_out * (*newOut)(struct mtar_io_out * io, const struct mtar_option * option);
	void (*showDescription)(void);
};

struct mtar_format_in * mtar_format_get_in(struct mtar_io_in * io, const struct mtar_option * option);
struct mtar_format_out * mtar_format_get_out(struct mtar_io_out * io, const struct mtar_option * option);
void mtar_format_register(struct mtar_format * format);

#endif

