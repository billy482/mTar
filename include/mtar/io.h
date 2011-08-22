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
*  Last modified: Mon, 22 Aug 2011 16:07:15 +0200                       *
\***********************************************************************/

#ifndef __MTAR_IO_H__
#define __MTAR_IO_H__

// off_t, ssize_t
#include <sys/types.h>

struct mtar_option;

/**
 * \addtogroup WrtIoMdl Writing IO module
 *
 * Writing an io module is quite easy but it takes a bit time.
 *
 * First, we need to write an implemtation of struct mtar_io_in, then write an
 * implemtation of mtar_io_out and finaly write an implemtation of mtar_io.
 *
 * An example :
 *   - An implementation of mtar_io : \ref mtar_io_file
 *   - An implementation of mtar_io_in : See code source of \ref module/io/file/in_ops.c
 *   - An implementation of mtar_io_out : See code source of \ref module/io/file/out_ops.c
 */

/**
 * \brief Used for input io
 */
struct mtar_io_in {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 * \note use \b last_errno to know why an error occured
	 */
	struct mtar_io_in_ops {
		/**
		 * \brief close the input stream
		 * \param[in] io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_in * io);
		/**
		 * \brief forward the stream to \b offset bytes
		 * \param[in] io : io module
		 * \param[in] offset :
		 * \return new offset position of stream in bytes
		 */
		off_t (*forward)(struct mtar_io_in * io, off_t offset);
		/**
		 * \brief Release all memory used by this module
		 * \param[in] io : io module
		 */
		void (*free)(struct mtar_io_in * io);
		/**
		 * \brief get the lastest error number
		 * \param[in] io : io module
		 * \return error number
		 */
		int (*last_errno)(struct mtar_io_in * io);
		/**
		 * \brief get the position of stream in bytes
		 * \param[in] io : io module
		 * \return position of stream
		 */
		off_t (*pos)(struct mtar_io_in * io);
		/**
		 * \brief read data from stream
		 * \param[in] io : instance of stream
		 * \param[out] data : read data to \b data
		 * \param[in] length : length of \b data
		 * \return length of data read
		 */
		ssize_t (*read)(struct mtar_io_in * io, void * data, ssize_t length);
	} * ops;
	/**
	 * \brief Private data used by io module
	 *
	 * Users should not modify this field
	 */
	void * data;
};

/**
 * \brief Used for output io
 */
struct mtar_io_out {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_io_out_ops {
		/**
		 * \brief close the input stream
		 * \param[in] io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_out * io);
		/**
		 * \brief foo
		 */
		int (*flush)(struct mtar_io_out * io);
		void (*free)(struct mtar_io_out * io);
		int (*last_errno)(struct mtar_io_out * io);
		off_t (*pos)(struct mtar_io_out * io);
		struct mtar_io_in * (*reopenForReading)(struct mtar_io_out * io, const struct mtar_option * option);
		ssize_t (*write)(struct mtar_io_out * io, const void * data, ssize_t length);
	} * ops;
	/**
	 * \brief Private data used by io module
	 */
	void * data;
};

/**
 * \brief IO driver
 */
struct mtar_io {
	/**
	 * \brief name of driver
	 */
	const char * name;
	/**
	 * \brief get a new input handler
	 * \param[in] fd : an opened file descriptor
	 * \param[in] flags : flags used to open file
	 * \param[in] option : a struct containing argument passed to \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_in
	 */
	struct mtar_io_in * (*new_in)(int fd, int flags, const struct mtar_option * option);
	/**
	 * \brief get a new output handler
	 * \param[in] fd : an opened file descriptor
	 * \param[in] flags : flags used to open file
	 * \param[in] option : a struct containing argument passed to \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_out
	 */
	struct mtar_io_out * (*new_out)(int fd, int flags, const struct mtar_option * option);
	/**
	 * \brief print a short description about driver
	 *
	 * This function is called by \b mtar when argument is --list-ios
	 */
	void (*show_description)(void);
};

/**
 * \brief helper function
 * \param[in] fd : an allready opened file descriptor
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_in * mtar_io_in_get_fd(int fd, int flags, const struct mtar_option * option);
/**
 * \brief helper function which open a file and load correct io module
 * \param[in] filename : name of an existing file
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_in * mtar_io_in_get_file(const char * filename, int flags, const struct mtar_option * option);
/**
 * \brief helper function
 * \param[in] fd : an allready opened file descriptor
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_out
 */
struct mtar_io_out * mtar_io_out_get_fd(int fd, int flags, const struct mtar_option * option);
/**
 * \brief helper function which open a file and load correct io module
 * \param[in] filename : name of an existing file
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_out
 */
struct mtar_io_out * mtar_io_out_get_file(const char * filename, int flags, const struct mtar_option * option);
/**
 * \brief register a new io driver
 * \param[in] io : io driver
 * \note io \b should be statically allocated
 * \bug this function do not check if there is two io driver with the same name
 * or if there is two same io driver
 */
void mtar_io_register(struct mtar_io * io);

#endif

