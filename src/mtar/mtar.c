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
*  Last modified: Fri, 29 Apr 2011 10:00:55 +0200                       *
\***********************************************************************/

#include "function.h"
#include "io.h"
#include "option.h"
#include "verbose.h"

int main(int argc, char ** argv) {
	static struct mtar_option option;
	int failed = mtar_option_parse(&option, argc, argv);
	if (failed)
		return failed;

	mtar_verbose_configure(&option);

	failed = mtar_option_check(&option);
	if (failed)
		return failed;

	struct mtar_io * io = mtar_io_get(&option);
	if (!io) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Failed to open file\n");
		return 2;
	}

	failed = option.doWork(io, &option);

	io->ops->free(io);
	mtar_option_free(&option);

	return failed;
}

