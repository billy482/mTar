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
*  Last modified: Tue, 03 May 2011 17:13:43 +0200                       *
\***********************************************************************/

#ifndef __MTAR_PLUGIN_H__
#define __MTAR_PLUGIN_H__

// ssize_t
#include <sys/types.h>

struct mtar_option;

struct mtar_plugin {
	const char * name;
	struct mtar_plugin_ops {
		int (*addFile)(struct mtar_plugin * p, const char * filename);
		int (*addLabel)(struct mtar_plugin * p, const char * label);
		int (*addLink)(struct mtar_plugin * p, const char * src, const char * target);
		int (*endOfFile)(struct mtar_plugin * p);
		void (*free)(struct mtar_plugin * p);
		ssize_t (*read)(struct mtar_plugin * p, const void * data, ssize_t length);
		ssize_t (*write)(struct mtar_plugin * p, const void * data, ssize_t length);
	} * ops;
	void * data;
};

typedef struct mtar_plugin * (*mtar_plugin_f)(const struct mtar_option * option);

void mtar_plugin_register(const char * name, mtar_plugin_f format);

void mtar_plugin_addFile(const char * filename);

#endif
