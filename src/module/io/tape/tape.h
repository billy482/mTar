/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar (modular tar) is free software; you can redistribute it and/or      *
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
*  Last modified: Wed, 16 May 2012 21:56:16 +0200                           *
\***************************************************************************/

#ifndef __MTAR_IO_TAPE_H__
#define __MTAR_IO_TAPE_H__

// off_t, ssize_t
#include <sys/types.h>

#include <mtar/io.h>

struct mtar_io_reader * mtar_io_tape_new_reader(int fd, int flags, const struct mtar_option * option);
struct mtar_io_writer * mtar_io_tape_new_writer(int fd, int flags, const struct mtar_option * option);
int mtar_io_tape_scsi_read_capacity(int fd, ssize_t * total_free, ssize_t * total);
int mtar_io_tape_scsi_read_position(int fd, off_t * position);

#endif

