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
*  Last modified: Tue, 30 Aug 2011 09:09:34 +0200                       *
\***********************************************************************/

// strcmp, strlen, strncmp, strrchr, strspn
#include <string.h>
// calloc, free, realloc
#include <stdlib.h>

#include <mtar/file.h>

#include "filter.h"
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

	if (option->owner) {
		uid_t uid = mtar_file_user2uid(option->owner);
		if (uid == (uid_t) -1) {
			mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Invalid user (%s)\n", option->owner);
			return 1;
		}
	}
	if (option->group) {
		gid_t gid = mtar_file_group2gid(option->group);
		if (gid == (gid_t) -1) {
			mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Invalid group (%s)\n", option->group);
			return 1;
		}
	}
	return 0;
}

void mtar_option_free(struct mtar_option * option) {
	// main operation mode
	option->doWork = 0;

	// overwrite control
	option->verify = 0;

	// device selection and switching
	option->filename = 0;

	// archive format selection
	option->format = 0;
	option->label = 0;

	// compression options
	option->compress_module = 0;

	// local file selections
	if (option->nbFiles > 0) {
		free(option->files);
		option->files = 0;
		option->nbFiles = 0;
	}
	option->working_directory = 0;

	// mtar specific option
	if (option->nb_plugins > 0) {
		free(option->plugins);
		option->plugins = 0;
		option->nb_plugins = 0;
	}
}

int mtar_option_parse(struct mtar_option * option, int argc, char ** argv) {
	// main operation mode
	option->doWork = 0;

	// overwrite control
	option->verify = 0;

	// handling of file attributes
	option->atime_preserve = MTAR_OPTION_ATIME_NONE;
	option->group = 0;
	option->owner = 0;

	// device selection and switching
	option->filename = 0;

	// device blocking
	option->block_factor = 10;

	// archive format selection
	option->format = "ustar";
	option->label = 0;

	// compression options
	option->compress_module = 0;
	option->compress_level = 6;

	// local file selections
	option->files = 0;
	option->nbFiles = 0;
	option->working_directory = 0;

	// informative output
	option->verbose = 0;

	// mtar specific option
	option->plugins = 0;
	option->nb_plugins = 0;


	if (argc < 2) {
		mtar_option_show_help(*argv);
		return 2;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-bcCfHjtvVWxz?");
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
				case 'b':
					option->block_factor = atoi(argv[optArg++]);
					break;

				case 'c':
					option->doWork = mtar_function_get("create");
					break;

				case 'C':
					option->working_directory = argv[optArg++];
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

				case 'H':
					option->format = argv[optArg++];
					break;

				case 'j':
					option->compress_module = "bzip2";
					break;

				case 't':
					option->doWork = mtar_function_get("list");
					break;

				case 'v':
					if (option->verbose < 2)
						option->verbose++;
					break;

				case 'V':
					option->label = argv[optArg++];
					break;

				case 'W':
					option->verify = 1;
					break;

				case 'x':
					option->doWork = mtar_function_get("extract");
					break;

				case 'z':
					option->compress_module = "gzip";
					break;

				case '?':
					mtar_option_show_help(*argv);
					return 1;
			}
		}
	} else {
		optArg = 1;
	}

	if (optArg < argc && !strncmp(argv[optArg], "--", 2)) {
		while (optArg < argc) {
			if (!strncmp(argv[optArg], "--atime-preserve", 16)) {
				option->atime_preserve = MTAR_OPTION_ATIME_REPLACE;
			} else if (!strncmp(argv[optArg], "--blocking-factor", 17)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (opt)
					option->block_factor = atoi(opt);
			} else if (!strcmp(argv[optArg], "--bzip2")) {
				option->compress_module = "bzip2";
			} else if (!strcmp(argv[optArg], "--create")) {
				option->doWork = mtar_function_get("create");
			} else if (!strncmp(argv[optArg], "--directory", 11)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (opt)
					option->working_directory = opt;
			} else if (!strncmp(argv[optArg], "--compression-level", 19)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				option->compress_level = atoi(opt);
			} else if (!strcmp(argv[optArg], "--extract")) {
				option->doWork = mtar_function_get("extract");
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
			} else if (!strncmp(argv[optArg], "--group", 7)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (opt)
					option->group = opt;
			} else if (!strcmp(argv[optArg], "--gzip")) {
				option->compress_module = "gzip";
			} else if (!strcmp(argv[optArg], "--help")) {
				mtar_option_show_help(*argv);
				return 1;
			} else if (!strncmp(argv[optArg], "--label", 7)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (opt)
					option->label = opt;
			} else if (!strcmp(argv[optArg], "--list")) {
				option->doWork = mtar_function_get("list");
			} else if (!strcmp(argv[optArg], "--list-filters")) {
				mtar_option_show_version(*argv);
				mtar_filter_show_description();
				return 1;
			} else if (!strcmp(argv[optArg], "--list-formats")) {
				mtar_option_show_version(*argv);
				mtar_format_show_description();
				return 1;
			} else if (!strcmp(argv[optArg], "--list-functions")) {
				mtar_option_show_version(*argv);
				mtar_function_show_description();
				return 1;
			} else if (!strcmp(argv[optArg], "--list-ios")) {
				mtar_option_show_version(*argv);
				mtar_io_show_description();
				return 1;
			} else if (!strncmp(argv[optArg], "--owner", 7)) {
				char * opt = strchr(argv[optArg], '=');
				if (opt)
					opt++;
				else if (optArg <= argc)
					opt = argv[++optArg];

				if (opt)
					option->owner = opt;
			} else if (!strcmp(argv[optArg], "--plugin")) {
				optArg++;

				option->plugins = realloc(option->plugins, (option->nb_plugins + 1) * sizeof(char *));
				option->plugins[option->nb_plugins] = argv[optArg];
				option->nb_plugins++;
			} else if (!strcmp(argv[optArg], "--verbose")) {
				if (option->verbose < 2)
					option->verbose++;
			} else if (!strcmp(argv[optArg], "--verify")) {
				option->verify = 1;
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
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Usage: mtar [short_option] [param_short_option] [long_option] [--] [files]\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Main operation mode:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -c, --create               : create new archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -t, --list                 : list files from tar archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -x, --extract              : extract new archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --function FUNCTION *      : use FUNCTION as action\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --function help=FUNCTION * : show specific help from function FUNCTION\n\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  where FUNCTION is one of:\n");
	mtar_function_show_description();

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\n  Main operation mode:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -W, --verify : attempt to verify the archive after writing it\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Handling of file attributes:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --group=NAME : force NAME as group for added files\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --owner=NAME : force NAME as owner for added files\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Device selection and switching:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -f, --file=ARCHIVE : use ARCHIVE file or device ARCHIVE\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Device blocking:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -b, --blocking-factor=BLOCKS : BLOCKS x 512 bytes per record\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Archive format selection:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -H, --format=FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -V, --label=TEXT    : create archive with volume name TEXT\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  where FORMAT is one of the following:\n");
	mtar_format_show_description();

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\n  Compression options:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -j, --bzip2                 : filter the archive through bzip2\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -z, --gzip                  : filter the archive through gzip\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --compression-level=LEVEL * : Set the level of compression (1 <= LEVEL <= 9)\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Local file selection:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -C, --directory=DIR : change to directory DIR\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Informative output:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -v, --verbose : verbosely list files processed\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Other options:\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -?, --help : give this help list\n\n");

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-filters *   : list available io filters\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-formats *   : list available format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-functions * : list available function\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --list-ios *       : list available io backend\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    --plugin PLUGIN *  : load a plugin which will interact with an function\n\n");

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

