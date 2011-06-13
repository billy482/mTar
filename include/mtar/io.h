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
*  Last modified: Wed, 08 Jun 2011 13:34:38 +0200                       *
\***********************************************************************/

#ifndef __MTAR_IO_H__
#define __MTAR_IO_H__

// off_t, ssize_t
#include <sys/types.h>

struct mtar_option;

struct mtar_io_in {
	const char * name;
	struct mtar_io_in_ops {
		int (*close)(struct mtar_io_in * io);
		off_t (*forward)(struct mtar_io_in * io, off_t offset);
		void (*free)(struct mtar_io_in * io);
		off_t (*pos)(struct mtar_io_in * io);
		ssize_t (*read)(struct mtar_io_in * io, void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_io_out {
	const char * name;
	struct mtar_io_out_ops {
		int (*close)(struct mtar_io_out * io);
		int (*flush)(struct mtar_io_out * io);
		void (*free)(struct mtar_io_out * io);
		off_t (*pos)(struct mtar_io_out * io);
		ssize_t (*write)(struct mtar_io_out * io, const void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_io {
	const char * name;
	struct mtar_io_in * (*newIn)(int fd, int flags, const struct mtar_option * option);
	struct mtar_io_out * (*newOut)(int fd, int flags, const struct mtar_option * option);
	void (*showDescription)(void);
};

struct mtar_io_in * mtar_io_in_get_fd(int fd, int flags, const struct mtar_option * option);
struct mtar_io_in * mtar_io_in_get_file(const char * filename, int flags, const struct mtar_option * option);
struct mtar_io_out * mtar_io_out_get_fd(int fd, int flags, const struct mtar_option * option);
struct mtar_io_out * mtar_io_out_get_file(const char * filename, int flags, const struct mtar_option * option);
void mtar_io_register(struct mtar_io * function);

#endif

