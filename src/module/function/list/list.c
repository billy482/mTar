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
*  Last modified: Wed, 06 Jul 2011 09:54:39 +0200                       *
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
static void mtar_function_list_init(void) __attribute__((constructor));
static void mtar_function_list_showDescription(void);
static void mtar_function_list_showHelp(void);

static struct mtar_function mtar_function_list_functions = {
	.name            = "list",
	.doWork          = mtar_function_list,
	.showDescription = mtar_function_list_showDescription,
	.showHelp        = mtar_function_list_showHelp,
};


int mtar_function_list(const struct mtar_option * option) {
	struct mtar_format_in * format = mtar_format_get_in(option);
	if (!format)
		return 1;

	struct mtar_format_header header;
	int failed;

	while (!(failed = format->ops->get_header(format, &header))) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %o\n", header.path, header.mode);

		format->ops->skip_file(format);
	}

	format->ops->free(format);

	return failed;
}

void mtar_function_list_init() {
	mtar_function_register(&mtar_function_list_functions);
}

void mtar_function_list_showDescription() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  list : List files from tar archive\n");
}

void mtar_function_list_showHelp() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  List files from tar archive\n");
}

