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
*  Last modified: Wed, 20 Apr 2011 22:56:14 +0200                       *
\***********************************************************************/

// strlen, strrchr, strspn
#include <string.h>

#include <mtar/io.h>
#include <mtar/option.h>

#include "function.h"
#include "io.h"
#include "verbose.h"

static void showHelp(const char * path);
static void showVersion(const char * path);

int main(int argc, char ** argv) {
	if (argc < 2) {
		showHelp(*argv);
		return 1;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-cfhvV");
	if (length != goodArg) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Invalid argument '%c'\n", argv[1][goodArg]);
		showHelp(*argv);
		return 1;
	}

	static struct mtar_option option;
	mtar_option_init(&option);

	static unsigned int i;
	static int optArg = 2;
	for (i = 0; i < length; i++) {
		switch (argv[1][i]) {
			case 'c':
				option.function = MTAR_FUNCTION_CREATE;
				option.doWork = mtar_function_get("create");
				break;

			case 'f':
				if (option.filename) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "File is already defined (%s)\n", option.filename);
					showHelp(*argv);
					return 1;
				}
				if (optArg >= argc) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Argument 'f' require a parameter\n");
					showHelp(*argv);
					return 1;
				}
				option.filename = argv[optArg++];
				break;

			case 'h':
				showHelp(*argv);
				return 0;

			case 'v':
				if (option.verbose < 2)
					option.verbose++;
				break;

			case 'V':
				showVersion(*argv);
				return 0;
		}
	}

	static int j;
	for (j = optArg; j < argc; j++)
		mtar_option_add_file(&option, argv[j]);

	mtar_verbose_configure(&option);

	static struct mtar_verbose verbose;
	//mtar_verbose_get(&verbose, &option);

	int failed = mtar_option_check(&option, &verbose);
	if (failed)
		return failed;

	struct mtar_io * io = mtar_io_get(&option, &verbose);
	if (!io) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Failed to open file\n");
		return 2;
	}

	failed = option.doWork(io, &option, &verbose);

	io->ops->free(io);

	return failed;
}

void showHelp(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s)\n", ptr, MTAR_VERSION);
}

void showVersion(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s, build: %s %s)\n", ptr, MTAR_VERSION, __DATE__, __TIME__);
}

