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
*  Last modified: Sat, 05 May 2012 14:42:10 +0200                           *
\***************************************************************************/

#ifndef __MTAR_PATTERN_H__
#define __MTAR_PATTERN_H__

// size_t
#include <stddef.h>

#define MTAR_PATTERN_API_VERSION 1

struct mtar_option;

struct mtar_pattern_exclude {
	struct mtar_pattern_exclude_ops {
		void (*free)(struct mtar_pattern_exclude * pattern);
		int (*match)(struct mtar_pattern_exclude * pattern, const char * filename);
	} * ops;
	void * data;
};

struct mtar_pattern_include {
	struct mtar_pattern_include_ops {
		void (*free)(struct mtar_pattern_include * pattern);
		int (*has_next)(struct mtar_pattern_include * pattern);
		void (*next)(struct mtar_pattern_include * pattern, char * filename, size_t length);
	} * ops;
	void * data;
};

enum mtar_pattern_option {
	MTAR_PATTERN_OPTION_ANCHORED    = 0x1,
	MTAR_PATTERN_OPTION_IGNORE_CASE = 0x2,
	MTAR_PATTERN_OPTION_RECURSION   = 0x4,

	MTAR_PATTERN_OPTION_DEFAULT_EXCLUDE = 0x0,
	MTAR_PATTERN_OPTION_DEFAULT_INCLUDE = MTAR_PATTERN_OPTION_RECURSION,
};

struct mtar_pattern_driver {
	const char * name;

	struct mtar_pattern_exclude * (*new_exclude)(const char * pattern, enum mtar_pattern_option option);
	struct mtar_pattern_include * (*new_include)(const char * pattern, enum mtar_pattern_option option);

	void (*show_description)(void);
	void (*show_version)(void);

	int api_version;
};

struct mtar_pattern_exclude * mtar_pattern_get_exclude(const char * engine, const char * pattern, enum mtar_pattern_option option);
struct mtar_pattern_include * mtar_pattern_get_include(const char * engine, const char * pattern, enum mtar_pattern_option option);
int mtar_pattern_match(const struct mtar_option * option, const char * filename);
void mtar_pattern_register(struct mtar_pattern_driver * driver);

#endif

