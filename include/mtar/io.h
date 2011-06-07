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
*  Last modified: Fri, 03 Jun 2011 22:50:09 +0200                       *
\***********************************************************************/

#ifndef __MTAR_IO_H__
#define __MTAR_IO_H__

// ssize_t
#include <sys/types.h>

struct mtar_option;

struct mtar_io {
	struct mtar_io_ops {
		int (*canSeek)(struct mtar_io * io);
		int (*close)(struct mtar_io * io);
		void (*free)(struct mtar_io * io);
		off_t (*pos)(struct mtar_io * io);
		ssize_t (*read)(struct mtar_io * io, void * data, ssize_t length);
		off_t (*seek)(struct mtar_io * io, off_t offset, int whence);
		ssize_t (*write)(struct mtar_io * io, const void * data, ssize_t length);
	} * ops;
	void * data;
};

typedef struct mtar_io * (*mtar_io_f)(int fd, int flags, const struct mtar_option * option);

struct mtar_io * mtar_io_get_fd(int fd, int flags, const struct mtar_option * option);
struct mtar_io * mtar_io_get_file(const char * filename, int flags, const struct mtar_option * option);
void mtar_io_register(const char * name, mtar_io_f function);


struct mtar_io_in {
	struct mtar_io_in_ops {
		int (*close)(struct mtar_io_in * io);
		void (*free)(struct mtar_io_in * io);
		off_t (*pos)(struct mtar_io_in * io);
		ssize_t (*read)(struct mtar_io_in * io, void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_io_out {
	struct mtar_io_out_ops {
		int (*close)(struct mtar_io * io);
		void (*free)(struct mtar_io * io);
		off_t (*pos)(struct mtar_io * io);
		ssize_t (*write)(struct mtar_io * io, const void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_io2 {
	const char * name;
	struct mtar_io_in * (*newIn)(struct mtar_io_in * io, const struct mtar_option * option);
};

#endif

