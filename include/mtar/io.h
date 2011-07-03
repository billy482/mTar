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
*  Last modified: Sat, 02 Jul 2011 08:16:08 +0200                       *
\***********************************************************************/

#ifndef __MTAR_IO_H__
#define __MTAR_IO_H__

// off_t, ssize_t
#include <sys/types.h>

struct mtar_option;

/**
 * \brief Used for input io
 */
struct mtar_io_in {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_io_in_ops {
		/**
		 * \brief close the input stream
		 * \param io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_in * io);
		off_t (*forward)(struct mtar_io_in * io, off_t offset);
		/**
		 * \brief Release all memory used by this module
		 */
		void (*free)(struct mtar_io_in * io);
		int (*last_errno)(struct mtar_io_in * io);
		off_t (*pos)(struct mtar_io_in * io);
		/**
		 * \brief read data from stream
		 * \param io : instance of stream
		 * \param data : read data to \b data
		 * \param length : length of \b data
		 * \return length of data read
		 */
		ssize_t (*read)(struct mtar_io_in * io, void * data, ssize_t length);
	} * ops;
	/**
	 * \brief Private data used by io module
	 */
	void * data;
};

/**
 * \brief Used for output io
 */
struct mtar_io_out {
	struct mtar_io_out_ops {
		/**
		 * \brief close the input stream
		 * \param io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_out * io);
		int (*flush)(struct mtar_io_out * io);
		void (*free)(struct mtar_io_out * io);
		int (*last_errno)(struct mtar_io_out * io);
		off_t (*pos)(struct mtar_io_out * io);
		ssize_t (*write)(struct mtar_io_out * io, const void * data, ssize_t length);
	} * ops;
	/**
	 * \brief Private data used by io module
	 */
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

