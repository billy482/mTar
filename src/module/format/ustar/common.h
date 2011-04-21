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
*  Last modified: Thu, 21 Apr 2011 23:04:39 +0200                       *
\***********************************************************************/

#ifndef __MTAR_FORMAT_USTAR_H__
#define __MTAR_FORMAT_USTAR_H__

#include <mtar/format.h>

struct mtar_format_ustar {
	struct mtar_io * io;
	unsigned short pos;
};

int mtar_format_ustar_addFile(struct mtar_format * f, const char * filename);
int mtar_format_ustar_addLabel(struct mtar_format * f, const char * filename);
int mtar_format_ustar_endOfFile(struct mtar_format * f);
void mtar_format_ustar_free(struct mtar_format * f);
int mtar_format_ustar_getHeader(struct mtar_format * f, struct mtar_format_header * header);
ssize_t mtar_format_ustar_read(struct mtar_format * f, void * data, ssize_t length);
ssize_t mtar_format_ustar_write(struct mtar_format * f, const void * data, ssize_t length);

#endif

