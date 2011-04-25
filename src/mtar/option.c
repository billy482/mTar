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
*  Last modified: Mon, 25 Apr 2011 20:23:30 +0200                       *
\***********************************************************************/

// strlen, strrchr, strspn
#include <string.h>
// realloc
#include <stdlib.h>

#include "function.h"
#include "option.h"
#include "verbose.h"

static void option_showHelp(const char * path);
static void option_showVersion(const char * path);


void option_showHelp(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s)\n", ptr, MTAR_VERSION);
}

void option_showVersion(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s, build: %s %s)\n", ptr, MTAR_VERSION, __DATE__, __TIME__);
}


int mtar_option_check(struct mtar_option * option) {
	if (!option->doWork) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "No function defined\n");
		return 1;
	}
	return 0;
}

int mtar_option_parse(struct mtar_option * option, int argc, char ** argv) {
	option->function = MTAR_FUNCTION_NONE;
	option->doWork = 0;

	option->format = "ustar";

	option->filename = 0;

	option->files = 0;
	option->nbFiles = 0;

	option->verbose = 0;

	if (argc < 2) {
		option_showHelp(*argv);
		return 1;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-cfhvV");
	if (length != goodArg) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Invalid argument '%c'\n", argv[1][goodArg]);
		option_showHelp(*argv);
		return 1;
	}

	static unsigned int i;
	static int optArg = 2;
	for (i = 0; i < length; i++) {
		switch (argv[1][i]) {
			case 'c':
				option->function = MTAR_FUNCTION_CREATE;
				option->doWork = mtar_function_get("create");
				break;

			case 'f':
				if (option->filename) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "File is already defined (%s)\n", option->filename);
					option_showHelp(*argv);
					return 1;
				}
				if (optArg >= argc) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Argument 'f' require a parameter\n");
					option_showHelp(*argv);
					return 1;
				}
				option->filename = argv[optArg++];
				break;

			case 'h':
				option_showHelp(*argv);
				return 0;

			case 'v':
				if (option->verbose < 2)
					option->verbose++;
				break;

			case 'V':
				option_showVersion(*argv);
				return 0;
		}
	}

	if (optArg < argc) {
		option->nbFiles = argc - optArg;
		option->files = calloc(option->nbFiles, sizeof(char *));
		for (i = 0; optArg < argc; i++, optArg++)
			option->files[i] = argv[optArg];
	}

	return 0;
}

