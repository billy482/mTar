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
*  Last modified: Sun, 28 Oct 2012 16:09:33 +0100                           *
\***************************************************************************/

#ifndef __MTAR_IO_H__
#define __MTAR_IO_H__

#include "plugin.h"

// off_t, ssize_t
#include <sys/types.h>

struct mtar_hashtable;
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
 *   - An implementation of mtar_io_reader : See code source of \ref module/io/file/reader.c
 *   - An implementation of mtar_io_writer : See code source of \ref module/io/file/writer.c
 */

/**
 * \brief Used for input io
 */
struct mtar_io_reader {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 * \note use \b last_errno to know why an error occured
	 */
	struct mtar_io_reader_ops {
		ssize_t (*block_size)(struct mtar_io_reader * io);
		/**
		 * \brief close the input stream
		 * \param[in] io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_reader * io);
		/**
		 * \brief forward the stream to \b offset bytes
		 * \param[in] io : io module
		 * \param[in] offset :
		 * \return new offset position of stream in bytes
		 */
		off_t (*forward)(struct mtar_io_reader * io, off_t offset);
		/**
		 * \brief Release all memory used by this module
		 * \param[in] io : io module
		 */
		void (*free)(struct mtar_io_reader * io);
		/**
		 * \brief get the lastest error number
		 * \param[in] io : io module
		 * \return error number
		 */
		int (*last_errno)(struct mtar_io_reader * io);
		/**
		 * \brief get the position of stream in bytes
		 * \param[in] io : io module
		 * \return position of stream
		 */
		off_t (*position)(struct mtar_io_reader * io);
		/**
		 * \brief read data from stream
		 * \param[in] io : instance of stream
		 * \param[out] data : read data to \b data
		 * \param[in] length : length of \b data
		 * \return length of data read
		 */
		ssize_t (*read)(struct mtar_io_reader * io, void * data, ssize_t length);
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
struct mtar_io_writer {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_io_writer_ops {
		ssize_t (*available_space)(struct mtar_io_writer * io);
		ssize_t (*block_size)(struct mtar_io_writer * io);
		/**
		 * \brief close the input stream
		 * \param[in] io : io module
		 * \return \b 0 if ok
		 */
		int (*close)(struct mtar_io_writer * io);
		/**
		 * \brief foo
		 */
		int (*flush)(struct mtar_io_writer * io);
		void (*free)(struct mtar_io_writer * io);
		int (*last_errno)(struct mtar_io_writer * io);
		ssize_t (*next_prefered_size)(struct mtar_io_writer * io);
		off_t (*position)(struct mtar_io_writer * io);
		struct mtar_io_reader * (*reopen_for_reading)(struct mtar_io_writer * io, const struct mtar_option * option);
		ssize_t (*write)(struct mtar_io_writer * io, const void * data, ssize_t length);
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
	 *
	 * \param[in] fd : an opened file descriptor
	 * \param[in] option : a struct containing argument passed to \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_in
	 */
	struct mtar_io_reader * (*new_reader)(int fd, const struct mtar_option * option, const struct mtar_hashtable * params);
	/**
	 * \brief get a new output handler
	 *
	 * \param[in] fd : an opened file descriptor
	 * \param[in] option : a struct containing argument passed to \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_out
	 */
	struct mtar_io_writer * (*new_writer)(int fd, const struct mtar_option * option, const struct mtar_hashtable * params);

	/**
	 * \brief print a short description about driver
	 *
	 * This function is called by \b mtar when argument is --list-ios
	 */
	void (*show_description)(void);
	/**
	 * \brief print a full version about driver
	 *
	 * This function is called by \b mtar when argument is --full-version
	 */
	void (*show_version)(void);

	struct mtar_plugin api_level;
};

/**
 * \brief helper function
 *
 * Find an appropriate io module which will use \a fd
 *
 * \param[in] fd : an already opened file descriptor
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_reader * mtar_io_reader_get_fd(int fd, const struct mtar_option * option, const struct mtar_hashtable * params);
/**
 * \brief helper function which open a file and load correct io module
 *
 * \param[in] filename : name of an existing file
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_reader * mtar_io_reader_get_file(const char * filename, int flags, const struct mtar_option * option, const struct mtar_hashtable * params);
/**
 * \brief helper function
 *
 * Find an appropriate io module which will use \a fd
 *
 * \param[in] fd : an allready opened file descriptor
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_out
 */
struct mtar_io_writer * mtar_io_writer_get_fd(int fd, const struct mtar_option * option, const struct mtar_hashtable * params);
/**
 * \brief helper function which open a file and load correct io module
 *
 * \param[in] filename : name of an existing file
 * \param[in] flags : flags used to open file
 * \param[in] option : a struct containing argument passed to \b mtar
 * \return 0 if failed or a new instance of struct mtar_io_out
 */
struct mtar_io_writer * mtar_io_writer_get_file(const char * filename, int flags, const struct mtar_option * option, const struct mtar_hashtable * params);
/**
 * \brief register a new io driver
 *
 * \param[in] io : io driver
 * \note io \b should be statically allocated
 */
void mtar_io_register(struct mtar_io * io);

#endif

