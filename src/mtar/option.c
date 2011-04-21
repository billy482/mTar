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
*  Last modified: Thu, 21 Apr 2011 22:10:48 +0200                       *
\***********************************************************************/

// realloc
#include <stdlib.h>

#include <mtar/verbose.h>

#include "option.h"

void mtar_option_add_file(struct mtar_option * option, const char * file) {
	option->files = realloc(option->files, (option->nbFiles + 1) * sizeof(char *));
	option->files[option->nbFiles] = file;
	option->nbFiles++;
}

int mtar_option_check(struct mtar_option * option) {
	if (!option->doWork) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "No function defined\n");
		return 1;
	}
	return 0;
}

void mtar_option_init(struct mtar_option * option) {
	option->function = MTAR_FUNCTION_NONE;
	option->doWork = 0;

	option->format = "ustar";

	option->filename = 0;

	option->files = 0;
	option->nbFiles = 0;

	option->verbose = 0;
}

