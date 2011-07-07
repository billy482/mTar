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
*  Last modified: Wed, 06 Jul 2011 09:51:51 +0200                       *
\***********************************************************************/

#include <mtar/verbose.h>

#include "common.h"

static void mtar_io_pipe_init(void) __attribute__((constructor));
static void mtar_io_pipe_show_description(void);

static struct mtar_io mtar_io_pipe = {
	.name             = "pipe",
	.new_in           = mtar_io_pipe_new_in,
	.new_out          = mtar_io_pipe_new_out,
	.show_description = mtar_io_pipe_show_description,
};


struct mtar_io * mtar_io_pipe_get_driver(void) {
	return &mtar_io_pipe;
}

void mtar_io_pipe_init() {
	mtar_io_register(&mtar_io_pipe);
}

void mtar_io_pipe_show_description() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  pipe : used for pipe (from file (mkfifo) or not)\n");
}

