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
*  Last modified: Mon, 13 Jun 2011 13:30:17 +0200                       *
\***********************************************************************/

#include "common.h"

static void mtar_io_pipe_showDescription(void);

static struct mtar_io mtar_io_pipe_ops = {
	.name            = "pipe",
	.newIn           = mtar_io_pipe_newIn,
	.newOut          = mtar_io_pipe_newOut,
	.showDescription = mtar_io_pipe_showDescription,
};


__attribute__((constructor))
static void mtar_io_file_init() {
	mtar_io_register(&mtar_io_pipe_ops);
}

void mtar_io_pipe_showDescription() { }
