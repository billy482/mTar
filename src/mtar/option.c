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
*  Last modified: Sun, 17 Jul 2011 21:32:34 +0200                       *
\***********************************************************************/

// strcmp, strlen, strncmp, strrchr, strspn
#include <string.h>
// calloc, free, realloc
#include <stdlib.h>

#include "format.h"
#include "function.h"
#include "io.h"
#include "option.h"
#include "verbose.h"

static void mtar_option_show_help(const char * path);
static void mtar_option_show_version(const char * path);


int mtar_option_check(struct mtar_option * option) {
	if (!option->doWork) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "No function defined\n");
		return 1;
	}
	return 0;
}

void mtar_option_free(struct mtar_option * option) {
	option->doWork = 0;
	option->format = 0;
	option->filename = 0;

	if (option->nbFiles > 0) {
		free(option->files);
		option->files = 0;
		option->nbFiles = 0;
	}

	if (option->nbPlugins > 0) {
		free(option->plugins);
		option->plugins = 0;
		option->nbPlugins = 0;
	}
}

int mtar_option_parse(struct mtar_option * option, int argc, char ** argv) {
	option->doWork = 0;

	option->format = "ustar";

	option->filename = 0;

	option->files = 0;
	option->nbFiles = 0;

	option->verbose = 0;

	option->compress_module = 0;
	option->compress_level = 6;

	option->plugins = 0;
	option->nbPlugins = 0;


	if (argc < 2) {
		mtar_option_show_help(*argv);
		return 2;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-cfhHtvVz");
	if (length != goodArg && strncmp(argv[1], "--", 2)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Invalid argument '%c'\n", argv[1][goodArg]);
		mtar_option_show_help(*argv);
		return 2;
	}

	unsigned int i;
	int optArg = 2;
	if (strncmp(argv[1], "--", 2)) {
		for (i = 0; i < length; i++) {
			switch (argv[1][i]) {
				case 'c':
					option->doWork = mtar_function_get("create");
					break;

				case 'f':
					if (option->filename) {
						mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "File is already defined (%s)\n", option->filename);
						mtar_option_show_help(*argv);
						return 2;
					}
					if (optArg >= argc) {
						mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Argument 'f' require a parameter\n");
						mtar_option_show_help(*argv);
						return 2;
					}
					option->filename = argv[optArg++];
					break;

				case 'h':
					mtar_option_show_help(*argv);
					return 1;

				case 'H':
					option->format = argv[optArg++];
					break;

				case 't':
					option->doWork = mtar_function_get("list");
					break;

				case 'v':
					if (option->verbose < 2)
						option->verbose++;
					break;

				case 'V':
					mtar_option_show_version(*argv);
					return 1;

				case 'z':
					option->compress_module = "gzip";
					break;
			}
		}
	} else {
		optArg = 1;
	}

	if (optArg < argc && !strncmp(argv[optArg], "--", 2)) {
		while (optArg < argc) {
			if (!strcmp(argv[optArg], "--create")) {
				option->doWork = mtar_function_get("create");
			} else if (!strncmp(argv[optArg], "--file", 6)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (option->filename) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "File is already defined (%s)\n", option->filename);
					mtar_option_show_help(*argv);
					return 2;
				}
				if (!opt) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Argument 'f' require a parameter\n");
					mtar_option_show_help(*argv);
					return 2;
				}
				option->filename = opt;
			} else if (!strncmp(argv[optArg], "--format", 8)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				option->format = opt;
			} else if (!strncmp(argv[optArg], "--function", 10)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (!strncmp(opt, "help=", 5)) {
					mtar_option_show_version(*argv);
					mtar_function_showHelp(strchr(opt, '=') + 1);
					return 1;
				} else {
					option->doWork = mtar_function_get(opt);
				}
			} else if (!strcmp(argv[optArg], "--help")) {
				mtar_option_show_help(*argv);
				return 1;
			} else if (!strcmp(argv[optArg], "--list")) {
				option->doWork = mtar_function_get("list");
			} else if (!strcmp(argv[optArg], "--list-filters")) {
				mtar_option_show_version(*argv);
				return 1;
			} else if (!strcmp(argv[optArg], "--list-formats")) {
				mtar_option_show_version(*argv);
				mtar_format_show_description();
				return 1;
			} else if (!strcmp(argv[optArg], "--list-functions")) {
				mtar_option_show_version(*argv);
				mtar_function_showDescription();
				return 1;
			} else if (!strcmp(argv[optArg], "--list-ios")) {
				mtar_option_show_version(*argv);
				mtar_io_show_description();
				return 1;
			} else if (!strcmp(argv[optArg], "--plugin")) {
				optArg++;

				option->plugins = realloc(option->plugins, (option->nbPlugins + 1) * sizeof(char *));
				option->plugins[option->nbPlugins] = argv[optArg];
				option->nbPlugins++;
			} else if (!strcmp(argv[optArg], "--gzip")) {
				option->compress_module = "gzip";
			} else if (!strcmp(argv[optArg], "--")) {
				optArg++;
				break;
			}

			optArg++;
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

void mtar_option_show_help(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s)\n", ptr, MTAR_VERSION);
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Usage: mtar [short_option] [param_short_option] [long_option] [--] [files]\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -c, --create          : create new archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -f, --file=ARCHIVE    : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -H, --format FORMAT * : use FORMAT as tar format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --function FUNCTION * : use FUNCTION as action\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "       help=FUNCTION      : show specific help from function FUNCTION\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-filters *      : list available io filters\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-formats *      : list available format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-functions *    : list available function\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-ios *          : list available io backend\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -h, --help            : show this and exit\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --plugin PLUGIN *     : load a plugin which will interact with an function\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -t, --list            : list files from tar archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -z, --gzip            : use gzip compression\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Parameters marked with * do not exist into gnu tar\n");
}

void mtar_option_show_version(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: modular tar (version: %s, build: %s %s)\n", ptr, MTAR_VERSION, __DATE__, __TIME__);
}

