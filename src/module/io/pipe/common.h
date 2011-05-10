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
*  Last modified: Tue, 10 May 2011 10:51:35 +0200                       *
\***********************************************************************/

#ifndef __MTAR_IO_PIPE_H__
#define __MTAR_IO_PIPE_H__

#include <mtar/io.h>

struct mtar_io_pipe {
	int fd;
	unsigned int pos;
};

int mtar_io_pipe_can_seek(struct mtar_io * io);
int mtar_io_pipe_close(struct mtar_io * io);
void mtar_io_pipe_free(struct mtar_io * io);
off_t mtar_io_pipe_pos(struct mtar_io * io);
ssize_t mtar_io_pipe_read(struct mtar_io * io, void * data, ssize_t length);
off_t mtar_io_pipe_seek(struct mtar_io * io, off_t offset, int whence);
ssize_t mtar_io_pipe_write(struct mtar_io * io, const void * data, ssize_t length);

#endif

