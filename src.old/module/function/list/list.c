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
*  Last modified: Thu, 26 May 2011 13:11:19 +0200                       *
\***********************************************************************/

// open
#include <fcntl.h>
// open
#include <sys/stat.h>
// open
#include <sys/types.h>

#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

static int mtar_function_list(const struct mtar_option * option);
static void mtar_function_list_showDescription(void);
static void mtar_function_list_showHelp(void);

static struct mtar_function mtar_function_list_functions = {
	.doWork          = mtar_function_list,
	.showDescription = mtar_function_list_showDescription,
	.showHelp        = mtar_function_list_showHelp,
};

__attribute__((constructor))
static void mtar_function_list_init() {
	mtar_function_register("list", &mtar_function_list_functions);
}


int mtar_function_list(const struct mtar_option * option) {
	struct mtar_io * io = 0;
	if (option->filename)
		io = mtar_io_get_file(option->filename, O_RDONLY, option);
	else
		io = mtar_io_get_file(0, O_RDONLY, option);
	if (!io)
		return 1;

	struct mtar_format * format = mtar_format_get(io, option);
	struct mtar_format_header header;
	int failed;

	while (!(failed = format->ops->getHeader(format, &header))) {

		//format->ops->skipFile(format);
	}

	format->ops->free(format);
	io->ops->free(io);

	return failed;
}

void mtar_function_list_showDescription() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  list : List files from tar archive\n");
}

void mtar_function_list_showHelp() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  List files from tar archive\n");
}

