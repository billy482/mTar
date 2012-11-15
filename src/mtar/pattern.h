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
*  Last modified: Thu, 15 Nov 2012 19:44:24 +0100                           *
\***************************************************************************/

#ifndef __MTAR_PATTERN_P_H__
#define __MTAR_PATTERN_P_H__

#include <mtar/pattern.h>

struct mtar_option;

enum mtar_pattern_tag_option {
	mtar_pattern_tag,
	mtar_pattern_tag_all,
	mtar_pattern_tag_under,
};

struct mtar_pattern_tag {
	char * tag;
	enum mtar_pattern_tag_option option;
};

struct mtar_pattern_exclude ** mtar_pattern_add_exclude(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option);
struct mtar_pattern_include ** mtar_pattern_add_include(struct mtar_pattern_include ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option);
struct mtar_pattern_exclude ** mtar_pattern_add_exclude_from_file(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, enum mtar_pattern_option option, const char * filename, struct mtar_option * op);
struct mtar_pattern_include ** mtar_pattern_add_include_from_file(struct mtar_pattern_include ** patterns, unsigned int * nb_patterns, char * engine, enum mtar_pattern_option option, const char * filename, struct mtar_option * op);
struct mtar_pattern_tag * mtar_pattern_add_tag(struct mtar_pattern_tag * tags, unsigned int * nb_tags, char * tag, enum mtar_pattern_tag_option option);
struct mtar_pattern_include * mtar_pattern_default_include_new(const char * pattern, enum mtar_pattern_option option);
void mtar_pattern_show_description(void);
void mtar_pattern_show_version(void);

#endif

