/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar (modular tar) is free software; you can redistribute it and/or      *
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
*  Copyright (C) 2012, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Tue, 23 Oct 2012 22:56:29 +0200                           *
\***************************************************************************/

#include <mtar-io-pipe.chcksum>
#include <mtar.version>

#include <mtar/verbose.h>

#include "pipe.h"

static void mtar_io_pipe_init(void) __attribute__((constructor));
static void mtar_io_pipe_show_description(void);
static void mtar_io_pipe_version(void);

static struct mtar_io mtar_io_pipe = {
	.name             = "pipe",

	.new_reader       = mtar_io_pipe_new_reader,
	.new_writer       = mtar_io_pipe_new_writer,

	.show_description = mtar_io_pipe_show_description,
	.show_version     = mtar_io_pipe_version,

	.api_level        = {
		.filter   = 0,
		.format   = 0,
		.function = 0,
		.io       = MTAR_IO_API_LEVEL,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = 0,
	},
};


static void mtar_io_pipe_init() {
	mtar_io_register(&mtar_io_pipe);
}

static void mtar_io_pipe_show_description() {
	mtar_verbose_print_help("pipe : used for pipe (from file (mkfifo) or system call (pipe))");
}

static void mtar_io_pipe_version() {
	mtar_verbose_printf("  pipe: used for pipe (from file (mkfifo) or system call (pipe)) (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("        SHA1 of source files: " MTAR_IO_PIPE_SRCSUM "\n");
}

