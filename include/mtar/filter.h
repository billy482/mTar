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
*  Last modified: Sun, 21 Oct 2012 10:24:10 +0200                           *
\***************************************************************************/

#ifndef __MTAR_FILTER_H__
#define __MTAR_FILTER_H__

#include "io.h"
#include "plugin.h"

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
	struct mtar_io_reader * (*new_reader)(struct mtar_io_reader * io, const struct mtar_option * option);
	/**
	 * \brief get a new output handler
	 * \param[in] option : a struct containing argument passed by \b mtar
	 * \return 0 if failed or a new instance of struct mtar_io_out
	 */
	struct mtar_io_writer * (*new_writer)(struct mtar_io_writer * io, const struct mtar_option * option);

	/**
	 * \brief print a short description about driver
	 *
	 * This function is called by \b mtar when argument is --list-filters
	 */
	void (*show_description)(void);
	/**
	 * \brief print extented version about driver
	 *
	 * This function is called by \b mtar when argument is --full-version
	 */
	void (*show_version)(void);

	/**
	 * \brief Requirement of filter's plugin
	 */
	const struct mtar_plugin api_level;
};

/**
 * \brief Get an instance of filter or io based on \a option parameter
 * \param[in] option : 
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_reader * mtar_filter_get_reader(const struct mtar_option * option);
/**
 * \brief Get an instance of filter or io based on \a option parameter
 * \param[in] io
 * \param[in] option : 
 * \return 0 if failed or a new instance of struct mtar_io_in
 */
struct mtar_io_reader * mtar_filter_get_reader2(struct mtar_io_reader * io, const struct mtar_option * option);
struct mtar_io_reader * mtar_filter_get_reader3(const char * filename, const struct mtar_option * option);
struct mtar_io_writer * mtar_filter_get_writer(const struct mtar_option * option);
struct mtar_io_writer * mtar_filter_get_writer2(struct mtar_io_writer * io, const struct mtar_option * option);
struct mtar_io_writer * mtar_filter_get_writer3(const char * filename, const struct mtar_option * option);

/**
 * \brief Register a filter io
 * \param[in] io : a filtering module io statically allocated
 */
void mtar_filter_register(struct mtar_filter * filter);

#endif

