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
*  Last modified: Wed, 20 Apr 2011 22:37:37 +0200                       *
\***********************************************************************/

#ifndef __MTAR_OPTION_H__
#define __MTAR_OPTION_H__

#include "common.h"
#include "function.h"

struct mtar_option {
	mtar_function_enum function;
	mtar_function doWork;

	const char * format;

	const char * filename;

	const char ** files;
	unsigned int nbFiles;

	enum mtar_verbose_level verbose;
};

void mtar_option_add_file(struct mtar_option * option, const char * file);
int mtar_option_check(struct mtar_option * option, struct mtar_verbose * verbose);
void mtar_option_init(struct mtar_option * option);

#endif

