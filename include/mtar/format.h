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
*  Last modified: Mon, 29 Aug 2011 10:25:54 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FORMAT_H__
#define __MTAR_FORMAT_H__

// dev_t, mode_t, ssize_t, time_t
#include <sys/types.h>

struct mtar_io_in;
struct mtar_io_out;
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
 *   - An implementation of mtar_format_in : See code source of \ref module/format/ustar/in_ops.c
 *   - An implementation of mtar_format_out : See code source of \ref module/format/ustar/out_ops.c
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
	char path[256];
	char * filename;
	/**
	 * \brief Value of hard or symbolic link
	 */
	char link[256];
	/**
	 * \brief Size of file
	 */
	ssize_t size;
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

enum mtar_format_in_header_status {
	MTAR_FORMAT_HEADER_OK,
	MTAR_FORMAT_HEADER_NOT_FOUND,
	MTAR_FORMAT_HEADER_BAD_HEADER,
	MTAR_FORMAT_HEADER_BAD_CHECKSUM,
};

/**
 * \brief Used for reading tar
 */
struct mtar_format_in {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_format_in_ops {
		/**
		 * \brief Release memory
		 * \param[in] f : a tar format
		 */
		void (*free)(struct mtar_format_in * f);
		/**
		 * \brief Parse the next header
		 * \param[in] f : a tar format
		 * \param[out] header : an already allocated header
		 */
		enum mtar_format_in_header_status (*get_header)(struct mtar_format_in * f, struct mtar_format_header * header);
		/**
		 * \brief Retreive the latest error
		 * \param[in] f : a tar format
		 */
		int (*last_errno)(struct mtar_format_in * f);
		ssize_t (*read)(struct mtar_format_in * f, void * data, ssize_t length);
		int (*skip_file)(struct mtar_format_in * f);
	} * ops;
	/**
	 * \brief Private data used by io module
	 */
	void * data;
};

struct mtar_format_out {
	struct mtar_format_out_ops {
		int (*add_file)(struct mtar_format_out * f, const char * filename);
		int (*add_label)(struct mtar_format_out * f, const char * label);
		int (*add_link)(struct mtar_format_out * f, const char * src, const char * target);
		ssize_t (*block_size)(struct mtar_format_out * f);
		int (*end_of_file)(struct mtar_format_out * f);
		void (*free)(struct mtar_format_out * f);
		int (*last_errno)(struct mtar_format_out * f);
		struct mtar_format_in * (*reopenForReading)(struct mtar_format_out * f, const struct mtar_option * option);
		ssize_t (*write)(struct mtar_format_out * f, const void * data, ssize_t length);
	} * ops;
	void * data;
};

struct mtar_format {
	const char * name;
	struct mtar_format_in * (*new_in)(struct mtar_io_in * io, const struct mtar_option * option);
	struct mtar_format_out * (*new_out)(struct mtar_io_out * io, const struct mtar_option * option);
	void (*show_description)(void);
};

struct mtar_format_in * mtar_format_get_in(const struct mtar_option * option);
struct mtar_format_in * mtar_format_get_in2(struct mtar_io_in * io, const struct mtar_option * option);
struct mtar_format_out * mtar_format_get_out(const struct mtar_option * option);
struct mtar_format_out * mtar_format_get_out2(struct mtar_io_out * io, const struct mtar_option * option);
void mtar_format_init_header(struct mtar_format_header * h);
void mtar_format_register(struct mtar_format * format);

#endif

