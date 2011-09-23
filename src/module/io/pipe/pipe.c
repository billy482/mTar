/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
*  modify it under the terms of the GNU General Public License              *
*  as published by the Free Software Foundation; either version 3           *
*  of the License, or (at your option) any later version.                   *
*                                                                           *
*  This program is distributed in the hope that it will be useful,          *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
*  GNU General Public License for more details.                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program; if not, write to the Free Software              *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                       *
*  Boston, MA  02110-1301, USA.                                             *
*                                                                           *
*  You should have received a copy of the GNU General Public License        *
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Fri, 23 Sep 2011 09:38:13 +0200                           *
\***************************************************************************/

#include <mtar/verbose.h>

#include "common.h"

static void mtar_io_pipe_init(void) __attribute__((constructor));
static void mtar_io_pipe_show_description(void);

static struct mtar_io mtar_io_pipe = {
	.name             = "pipe",
	.new_in           = mtar_io_pipe_new_in,
	.new_out          = mtar_io_pipe_new_out,
	.show_description = mtar_io_pipe_show_description,
	.api_version      = MTAR_IO_API_VERSION,
};


void mtar_io_pipe_init() {
	mtar_io_register(&mtar_io_pipe);
}

void mtar_io_pipe_show_description() {
	mtar_verbose_printf("used for pipe (from file (mkfifo) or system call (pipe))\n");
}

