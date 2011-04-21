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
*  Last modified: Thu, 21 Apr 2011 23:16:42 +0200                       *
\***********************************************************************/

// malloc
#include <stdlib.h>
// strncpy
#include <string.h>
// access
#include <unistd.h>

#include <mtar/verbose.h>

#include "common.h"

struct ustar {
	char filename[100];
	char filemode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char flag;
	char linkname[100];
	char magic[8];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};


int mtar_format_ustar_addFile(struct mtar_format * f, const char * filename) {
	if (access(filename, F_OK)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Can access to file: %s\n", filename);
		return 1;
	}

	struct ustar * header = malloc(512);

	strncpy(header->filename, filename, 100);
}

