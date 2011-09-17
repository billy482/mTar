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
*  Last modified: Sat, 17 Sep 2011 20:52:09 +0200                       *
\***********************************************************************/

// BZ2_bzlibVersion
#include <bzlib.h>

#include <mtar/verbose.h>

#include "common.h"

static void mtar_filter_bzip2_init(void) __attribute__((constructor));
static void mtar_filter_bzip2_show_description(void);

static struct mtar_filter mtar_filter_bzip2 = {
	.name             = "bzip2",
	.new_in           = mtar_filter_bzip2_new_in,
	.new_out          = mtar_filter_bzip2_new_out,
	.show_description = mtar_filter_bzip2_show_description,
};


void mtar_filter_bzip2_init() {
	mtar_filter_register(&mtar_filter_bzip2);
}

void mtar_filter_bzip2_show_description() {
	mtar_verbose_printf("filter from/to compressed data (using libbz2: v%s)\n", BZ2_bzlibVersion());
}

