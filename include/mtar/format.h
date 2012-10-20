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
*  Last modified: Sat, 20 Oct 2012 13:34:31 +0200                           *
\***************************************************************************/

#ifndef __MTAR_FORMAT_H__
#define __MTAR_FORMAT_H__

// dev_t, mode_t, ssize_t, time_t
#include <sys/types.h>

#include "plugin.h"

struct mtar_io_reader;
struct mtar_io_writer;
struct mtar_option;

/**
 * \addtogroup WrtFrmtMdl Writing format module
 *
 * Writing a format module is quite easy but it takes a bit time.
 *
 * First, we need to write an implemtation of struct mtar_format_in, then write an
 * implemtation of mtar_format_out and finaly write an implemtation of mtar_format.
 *
 * An example :
 *   - An implementation of mtar_format : See code source of \ref src/module/format/ustar/ustar.c
 *   - An implementation of mtar_format_reader : See code source of \ref module/format/ustar/in_ops.c
 *   - An implementation of mtar_format_writer : See code source of \ref module/format/ustar/out_ops.c
 */

/**
 * \brief Define a generic tar header
 */
struct mtar_format_header {
	/**
	 * \brief Id of device
	 *
	 * Used only by character or block device
	 *
	 * \code
	 * int major = dev >> 8;
	 * int minor = dev & 0xFF;
	 * \endcode
	 */
	dev_t dev;
	/**
	 * \brief A partial filename as recorded into tar
	 */
	char * path;
	/**
	 * \brief Value of hard or symbolic link
	 */
	char * link;
	/**
	 * \brief Size of file
	 */
	ssize_t size;
	ssize_t position;
	/**
	 * \brief List of permission
	 */
	mode_t mode;
	/**
	 * \brief Modified time
	 */
	time_t mtime;
	/**
	 * \brief User's id
	 * \note Id of user where tar has been made.
	 */
	uid_t uid;
	/**
	 * \brief User's name
	 * \note Name of user where tar has been made.
	 */
	char uname[32];
	/**
	 * \brief Group's id
	 * \note Id of group where tar has been made.
	 */
	gid_t gid;
	/**
	 * \brief Group's name
	 * \note Name of group where tar has been made.
	 */
	char gname[32];
	char is_label;
};

enum mtar_format_reader_header_status {
	mtar_format_header_bad_checksum,
	mtar_format_header_bad_header,
	mtar_format_header_end_of_tape,
	mtar_format_header_error,
	mtar_format_header_not_found,
	mtar_format_header_ok,
};

enum mtar_format_writer_status {
	mtar_format_writer_end_of_tape,
	mtar_format_writer_error,
	mtar_format_writer_ok,
};

/**
 * \brief Used for reading tar
 */
struct mtar_format_reader {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_format_reader_ops {
		/**
		 * \brief Release memory
		 * \param[in] f : a tar format
		 */
		void (*free)(struct mtar_format_reader * f);
		/**
		 * \brief Parse the next header
		 * \param[in] f : a tar format
		 * \param[out] header : an already allocated header
		 */
		enum mtar_format_reader_header_status (*get_header)(struct mtar_format_reader * f, struct mtar_format_header * header);
		/**
		 * \brief Retreive the latest error
		 * \param[in] f : a tar format
		 */
		int (*last_errno)(struct mtar_format_reader * f);
		void (*next_volume)(struct mtar_format_reader * f, struct mtar_io_reader * new_volume);
		ssize_t (*read)(struct mtar_format_reader * f, void * data, ssize_t length);
		enum mtar_format_reader_header_status (*skip_file)(struct mtar_format_reader * f);
	} * ops;
	/**
	 * \brief Private data used by io module
	 */
	void * data;
};

struct mtar_format_writer {
	struct mtar_format_writer_ops {
		enum mtar_format_writer_status (*add_file)(struct mtar_format_writer * f, const char * filename, struct mtar_format_header * header);
		enum mtar_format_writer_status (*add_label)(struct mtar_format_writer * f, const char * label);
		enum mtar_format_writer_status (*add_link)(struct mtar_format_writer * f, const char * src, const char * target, struct mtar_format_header * header);
		ssize_t (*available_space)(struct mtar_format_writer * io);
		ssize_t (*block_size)(struct mtar_format_writer * f);
		int (*end_of_file)(struct mtar_format_writer * f);
		void (*free)(struct mtar_format_writer * f);
		int (*last_errno)(struct mtar_format_writer * f);
		ssize_t (*next_prefered_size)(struct mtar_format_writer * f);
		void (*new_volume)(struct mtar_format_writer * f, struct mtar_io_writer * file);
		off_t (*position)(struct mtar_format_writer * io);
		struct mtar_format_reader * (*reopen_for_reading)(struct mtar_format_writer * f, const struct mtar_option * option);
		int (*restart_file)(struct mtar_format_writer * f, const char * filename, ssize_t position);
		ssize_t (*write)(struct mtar_format_writer * f, const void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_format {
	const char * name;

	struct mtar_format_reader * (*new_reader)(struct mtar_io_reader * io, const struct mtar_option * option);
	struct mtar_format_writer * (*new_writer)(struct mtar_io_writer * io, const struct mtar_option * option);

	void (*show_description)(void);
	void (*show_version)(void);

	struct mtar_plugin api_level;
};

#define MTAR_FORMAT_API_LEVEL 1

struct mtar_format_reader * mtar_format_get_reader(const struct mtar_option * option);
struct mtar_format_reader * mtar_format_get_reader2(struct mtar_io_reader * io, const struct mtar_option * option);
struct mtar_format_writer * mtar_format_get_writer(const struct mtar_option * option);
struct mtar_format_writer * mtar_format_get_writer2(struct mtar_io_writer * io, const struct mtar_option * option);
void mtar_format_free_header(struct mtar_format_header * h);
void mtar_format_init_header(struct mtar_format_header * h);
void mtar_format_register(struct mtar_format * format);

#endif

