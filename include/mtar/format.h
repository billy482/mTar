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
*  Last modified: Thu, 07 Jul 2011 22:54:33 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FORMAT_H__
#define __MTAR_FORMAT_H__

// dev_t, mode_t, ssize_t, time_t
#include <sys/types.h>

#include "option.h"

struct mtar_io_in;
struct mtar_io_out;
struct mtar_option;

/**
 * \brief Define a generic tar header
 */
struct mtar_format_header {
	dev_t dev;
	char path[256];
	char * filename;
	char link[256];
	ssize_t size;
	mode_t mode;
	time_t mtime;
	uid_t uid;
	char uname[32];
	gid_t gid;
	char gname[32];
};

/**
 * \brief Used for reading tar
 */
struct mtar_format_in {
	/**
	 * \brief This structure contains only functions pointers used as methods
	 */
	struct mtar_format_in_ops {
		void (*free)(struct mtar_format_in * f);
		int (*get_header)(struct mtar_format_in * f, struct mtar_format_header * header);
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
		int (*end_of_file)(struct mtar_format_out * f);
		void (*free)(struct mtar_format_out * f);
		int (*last_errno)(struct mtar_format_out * f);
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
struct mtar_format_out * mtar_format_get_out(const struct mtar_option * option);
void mtar_format_init_header(struct mtar_format_header * h);
void mtar_format_register(struct mtar_format * format);

#endif

