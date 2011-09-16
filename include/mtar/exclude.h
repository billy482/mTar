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
*  Last modified: Fri, 16 Sep 2011 11:15:55 +0200                       *
\***********************************************************************/

#ifndef __MTAR_EXCLUDE_H__
#define __MTAR_EXCLUDE_H__

struct mtar_option;

struct mtar_exclude {
	const char ** excludes;
	unsigned int nb_excludes;
	struct mtar_exclude_ops {
		int (*filter)(struct mtar_exclude * ex, const char * filename);
		void (*free)(struct mtar_exclude * ex);
	} * ops;
	void * data;
};

struct mtar_exclude_driver {
	const char * name;
	struct mtar_exclude * (*new)(const struct mtar_option * option);
	void (*show_description)(void);
};

int mtar_exclude_filter(const char * filename, const struct mtar_option * option);
struct mtar_exclude * mtar_exclude_get(const struct mtar_option * option);
void mtar_exclude_register(struct mtar_exclude_driver * ex);

#endif

