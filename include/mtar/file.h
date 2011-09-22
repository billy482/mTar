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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Thu, 22 Sep 2011 10:21:37 +0200                           *
\***************************************************************************/

#ifndef __MTAR_FILE_H__
#define __MTAR_FILE_H__

struct mtar_option;

// gid_t, mode_t, ssize_t, uid_t
#include <sys/types.h>

const char ** mtar_file_add_from_file(const char * filename, const char ** files, unsigned int * nbFiles, struct mtar_option * option);

/**
 * \brief Convert a file mode to \b buffer with `ls -l` style
 * \param[out] buffer : a 10 bytes already allocated buffer
 * \param[in] mode : convert with this mode
 */
void mtar_file_convert_mode(char * buffer, mode_t mode);

/**
 * \brief Convert an gid to group's name
 * \param[out] name : write group's name into it
 * \param[in] namelength : length of \b name
 * \param[in] gid : a gid
 * \note For better performance, this function maintain a cache
 */
void mtar_file_gid2name(char * name, ssize_t namelength, gid_t gid);

gid_t mtar_file_group2gid(const char * group);

/**
 * \brief Convert an uid to user's name
 * \param[out] name : write user's name into it
 * \param[in] namelength : length of \b name
 * \param[in] uid : a uid
 * \note For better performance, this function maintain a cache
 */
void mtar_file_uid2name(char * name, ssize_t namelength, uid_t uid);

uid_t mtar_file_user2uid(const char * user);

#endif

