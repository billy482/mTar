/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Last modified: Fri, 18 May 2012 22:19:54 +0200                           *
\***************************************************************************/

#ifndef __MTAR_IO_PIPE_H__
#define __MTAR_IO_PIPE_H__

#include <mtar/io.h>

struct mtar_io_pipe {
	int fd;
	off_t position;
	int last_errno;
};

ssize_t mtar_io_pipe_common_block_size(struct mtar_io_pipe * file);
int mtar_io_pipe_common_close(struct mtar_io_pipe * file);

struct mtar_io_in * mtar_io_pipe_new_in(int fd, int flags, const struct mtar_option * option);
struct mtar_io_out * mtar_io_pipe_new_out(int fd, int flags, const struct mtar_option * option);

#endif

