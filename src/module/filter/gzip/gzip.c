/***********************************************************************\
*       ______           ____     ___           __   _                  *
*      / __/ /____  ____/  _/__ _/ _ | ________/ /  (_)  _____ ____     *
*     _\ \/ __/ _ \/ __// // _ `/ __ |/ __/ __/ _ \/ / |/ / -_) __/     *
*    /___/\__/\___/_/ /___/\_, /_/ |_/_/  \__/_//_/_/|___/\__/_/        *
*                           /_/                                         *
*  -------------------------------------------------------------------  *
*  This file is a part of StorIqArchiver                                *
*                                                                       *
*  StorIqArchiver is free software; you can redistribute it and/or      *
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
*  Copyright (C) 2010, Clercin guillaume <gclercin@intellique.com>      *
*  Last modified: Mon, 18 Jul 2011 08:26:06 +0200                       *
\***********************************************************************/

#include <mtar/verbose.h>

#include "common.h"

static void mtar_filter_gzip_init(void) __attribute__((constructor));
static void mtar_filter_gzip_show_description(void);

static struct mtar_filter mtar_filter_gzip = {
	.name             = "gzip",
	.new_in           = mtar_filter_gzip_new_in,
	.new_out          = mtar_filter_gzip_new_out,
	.show_description = mtar_filter_gzip_show_description,
};


void mtar_filter_gzip_init() {
	mtar_filter_register(&mtar_filter_gzip);
}

void mtar_filter_gzip_show_description() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  gzip : filter from/to compressed data\n");
}

