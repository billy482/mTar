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
*  Last modified: Wed, 06 Jul 2011 09:50:11 +0200                       *
\***********************************************************************/

#include <mtar/verbose.h>

#include "common.h"

static void format_init(void) __attribute__((constructor));
static void mtar_format_show_description(void);

static struct mtar_format mtar_format_ustar = {
	.name             = "ustar",
	.new_in           = mtar_format_ustar_new_in,
	.new_out          = mtar_format_ustar_new_out,
	.show_description = mtar_format_show_description,
};

void format_init() {
	mtar_format_register(&mtar_format_ustar);
}

void mtar_format_show_description() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  ustar : default format in gnu tar\n");
}

