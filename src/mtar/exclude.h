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
*  Last modified: Sat, 17 Sep 2011 13:59:43 +0200                       *
\***********************************************************************/

#ifndef __MTAR_EXCLUDE_P_H__
#define __MTAR_EXCLUDE_P_H__

#include <mtar/exclude.h>

enum mtar_exclude_tag_option {
	MTAR_EXCLUDE_TAG,
	MTAR_EXCLUDE_TAG_ALL,
	MTAR_EXCLUDE_TAG_UNDER,
};

struct mtar_exclude_tag {
	const char * tag;
	enum mtar_exclude_tag_option option;
};

struct mtar_exclude_tag * mtar_exclude_add_tag(struct mtar_exclude_tag * tags, unsigned int * nb_tags, const char * tag, enum mtar_exclude_tag_option option);
void mtar_exclude_show_description(void);

#endif

