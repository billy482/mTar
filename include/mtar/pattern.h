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
*  Last modified: Sat, 20 Oct 2012 10:43:05 +0200                           *
\***************************************************************************/

#ifndef __MTAR_PATTERN_H__
#define __MTAR_PATTERN_H__

// bool
#include <stdbool.h>
// size_t
#include <stddef.h>

#include "plugin.h"

#define MTAR_PATTERN_API_LEVEL 1

struct mtar_option;

struct mtar_pattern_exclude {
	struct mtar_pattern_exclude_ops {
		void (*free)(struct mtar_pattern_exclude * pattern);
		bool (*match)(struct mtar_pattern_exclude * pattern, const char * filename);
	} * ops;
	void * data;
};

struct mtar_pattern_include {
	struct mtar_pattern_include_ops {
		void (*free)(struct mtar_pattern_include * pattern);
		bool (*has_next)(struct mtar_pattern_include * pattern, const struct mtar_option * option);
		void (*next)(struct mtar_pattern_include * pattern, char ** filename);
	} * ops;
	void * data;
};

enum mtar_pattern_option {
	mtar_pattern_option_anchored    = 0x1,
	mtar_pattern_option_ignore_case = 0x2,
	mtar_pattern_option_recursion   = 0x4,

	mtar_pattern_option_default_exclude = 0x0,
	mtar_pattern_option_default_include = mtar_pattern_option_recursion,
};

struct mtar_pattern_driver {
	const char * name;

	struct mtar_pattern_exclude * (*new_exclude)(const char * pattern, enum mtar_pattern_option option);
	struct mtar_pattern_include * (*new_include)(const char * pattern, enum mtar_pattern_option option);

	void (*show_description)(void);
	void (*show_version)(void);

	struct mtar_plugin api_level;
};

struct mtar_pattern_exclude * mtar_pattern_get_exclude(const char * engine, const char * pattern, enum mtar_pattern_option option);
struct mtar_pattern_include * mtar_pattern_get_include(const char * engine, const char * pattern, enum mtar_pattern_option option);
bool mtar_pattern_match(const struct mtar_option * option, const char * filename);
void mtar_pattern_register(struct mtar_pattern_driver * driver);

#endif

