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
*  Last modified: Mon, 12 Sep 2011 18:47:03 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FILTER_H__
#define __MTAR_FILTER_H__

#include "io.h"

/**
 * \brief Used by the driver of filter
 */
struct mtar_filter {
	/**
	 * \brief name of driver
	 *
	 * Should be uniq
	 */
	const char * name;
	/**
	 * \brief get a new input handler
	 * \param[in] option : a struct containing argument passed by \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_in
	 */
	struct mtar_io_in * (*new_in)(struct mtar_io_in * io, const struct mtar_option * option);
	/**
	 * \brief get a new output handler
	 * \param[in] option : a struct containing argument passed by \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_out
	 */
	struct mtar_io_out * (*new_out)(struct mtar_io_out * io, const struct mtar_option * option);
	/**
	 * \brief print a short description about driver
	 *
	 * This function is called by \b mtar when argument is --list-filters
	 */
	void (*show_description)(void);
};

/**
 * \brief Get an instance of filter or io based on \a option parameter
 * \param[in] option : 
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_in * mtar_filter_get_in(const struct mtar_option * option);
/**
 * \brief Get an instance of filter or io based on \a option parameter
 * \param[in] io
 * \param[in] option : 
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_in * mtar_filter_get_in2(struct mtar_io_in * io, const struct mtar_option * option);
struct mtar_io_in * mtar_filter_get_in3(const char * filename, const struct mtar_option * option);
struct mtar_io_out * mtar_filter_get_out(const struct mtar_option * option);
struct mtar_io_out * mtar_filter_get_out2(struct mtar_io_out * io, const struct mtar_option * option);
struct mtar_io_out * mtar_filter_get_out3(const char * filename, const struct mtar_option * option);

/**
 * \brief Register a filter io
 * \param[in] io : a filtering module io statically allocated
 */
void mtar_filter_register(struct mtar_filter * filter);

#endif

