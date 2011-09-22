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
*  Last modified: Thu, 22 Sep 2011 10:21:37 +0200                           *
\***************************************************************************/

// getopt_long
#include <getopt.h>
// strlen, strncmp, strrchr, strspn
#include <string.h>
// sscanf
#include <stdio.h>
// atoi, calloc, free, realloc
#include <stdlib.h>

#include <mtar/file.h>
#include <mtar/verbose.h>

#include "exclude.h"
#include "filter.h"
#include "format.h"
#include "function.h"
#include "io.h"
#include "option.h"

static void mtar_option_show_help(const char * path);
static void mtar_option_show_version(const char * path);


int mtar_option_check(struct mtar_option * option) {
	if (!option->doWork) {
		mtar_verbose_printf("No function defined\n");
		return 1;
	}

	if (option->owner) {
		uid_t uid = mtar_file_user2uid(option->owner);
		if (uid == (uid_t) -1) {
			mtar_verbose_printf("Invalid user (%s)\n", option->owner);
			return 1;
		}
	}
	if (option->group) {
		gid_t gid = mtar_file_group2gid(option->group);
		if (gid == (gid_t) -1) {
			mtar_verbose_printf("Invalid group (%s)\n", option->group);
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
	if (option->nb_excludes > 0) {
		free(option->excludes);
		option->excludes = 0;
		option->nb_excludes = 0;
	}
	if (option->nb_exclude_tags > 0) {
		free(option->exclude_tags);
		option->exclude_tags = 0;
		option->nb_exclude_tags = 0;
	}

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
	option->mode = 0;
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
	option->exclude_engine = "fnmatch";
	option->excludes = 0;
	option->nb_excludes = 0;
	option->exclude_option = MTAR_EXCLUDE_OPTION_DEFAULT;
	option->exclude_tags = 0;
	option->nb_exclude_tags = 0;
	option->delimiter = '\n';

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
	size_t goodArg = strspn(argv[1], "-bcCfHjtTvVWxXz?");
	if (length != goodArg && strncmp(argv[1], "--", 2)) {
		mtar_verbose_printf("Invalid argument '%c'\n", argv[1][goodArg]);
		mtar_option_show_help(*argv);
		return 2;
	}

	unsigned int i;
	if (strncmp(argv[1], "--", 2)) {
		optind = 2;
		for (i = 0; i < length; i++) {
			switch (argv[1][i]) {
				case 'b':
					option->block_factor = atoi(argv[optind++]);
					break;

				case 'c':
					option->doWork = mtar_function_get("create");
					break;

				case 'C':
					option->working_directory = argv[optind++];
					break;

				case 'f':
					if (option->filename) {
						mtar_verbose_printf("File is already defined (%s)\n", option->filename);
						mtar_option_show_help(*argv);
						return 2;
					}
					if (optind >= argc) {
						mtar_verbose_printf("Argument 'f' require a parameter\n");
						mtar_option_show_help(*argv);
						return 2;
					}
					option->filename = argv[optind++];
					break;

				case 'H':
					option->format = argv[optind++];
					break;

				case 'j':
					option->compress_module = "bzip2";
					break;

				case 't':
					option->doWork = mtar_function_get("list");
					break;

				case 'T':
					option->files = mtar_file_add_from_file(argv[optind++], option->files, &option->nbFiles, option);
					break;

				case 'v':
					if (option->verbose < 2)
						option->verbose++;
					break;

				case 'V':
					option->label = argv[optind++];
					break;

				case 'W':
					option->verify = 1;
					break;

				case 'x':
					option->doWork = mtar_function_get("extract");
					break;

				case 'X':
					option->excludes = mtar_file_add_from_file(argv[optind++], option->excludes, &option->nb_excludes, option);
					break;

				case 'z':
					option->compress_module = "gzip";
					break;

				case '?':
					mtar_option_show_help(*argv);
					return 1;
			}
		}
	}

	enum {
		OPT_BLOCKING_FACTOR = 'b',
		OPT_BZIP2           = 'j',
		OPT_CREATE          = 'c',
		OPT_DIRECTORY       = 'C',
		OPT_EXCLUDE_FROM    = 'X',
		OPT_EXTRACT         = 'x',
		OPT_FILE            = 'f',
		OPT_FILES_FROM      = 'T',
		OPT_FORMAT          = 'H',
		OPT_GZIP            = 'z',
		OPT_HELP            = '?',
		OPT_LABEL           = 'V',
		OPT_LIST            = 't',
		OPT_VERBOSE         = 'v',
		OPT_VERIFY          = 'W',

		OPT_ADD_FILE = 256,
		OPT_ATIME_PRESERVE,
		OPT_COMPRESSION_LEVEL,
		OPT_EXCLUDE,
		OPT_EXCLUDE_BACKUPS,
		OPT_EXCLUDE_CACHES,
		OPT_EXCLUDE_CACHES_ALL,
		OPT_EXCLUDE_CACHES_UNDER,
		OPT_EXCLUDE_ENGINE,
		OPT_EXCLUDE_TAG,
		OPT_EXCLUDE_TAG_ALL,
		OPT_EXCLUDE_TAG_UNDER,
		OPT_EXCLUDE_VCS,
		OPT_FUNCTION,
		OPT_GROUP,
		OPT_LIST_FILTERS,
		OPT_LIST_FORMATS,
		OPT_LIST_FUNCTINOS,
		OPT_LIST_IOS,
		OPT_MODE,
		OPT_NO_NULL,
		OPT_NULL,
		OPT_OWNER,
		OPT_PLUGIN,
		OPT_VERSION,
	};

	static struct option long_options[] = {
		{"add-file",             1, 0, OPT_ADD_FILE},
		{"atime-preserve",       2, 0, OPT_ATIME_PRESERVE},
		{"blocking-factor",      1, 0, OPT_BLOCKING_FACTOR},
		{"bzip2",                0, 0, OPT_BZIP2},
		{"compression-level",    1, 0, OPT_COMPRESSION_LEVEL},
		{"create",               0, 0, OPT_CREATE},
		{"directory",            1, 0, OPT_DIRECTORY},
		{"exclude",              1, 0, OPT_EXCLUDE},
		{"exclude-backups",      0, 0, OPT_EXCLUDE_BACKUPS},
		{"exclude-caches",       0, 0, OPT_EXCLUDE_CACHES},
		{"exclude-caches-all",   0, 0, OPT_EXCLUDE_CACHES_ALL},
		{"exclude-caches-under", 0, 0, OPT_EXCLUDE_CACHES_UNDER},
		{"exclude-engine",       1, 0, OPT_EXCLUDE_ENGINE},
		{"exclude-from",         1, 0, OPT_EXCLUDE_FROM},
		{"exclude-tag",          1, 0, OPT_EXCLUDE_TAG},
		{"exclude-tag-all",      1, 0, OPT_EXCLUDE_TAG_ALL},
		{"exclude-tag-under",    1, 0, OPT_EXCLUDE_TAG_UNDER},
		{"exclude-vcs",          0, 0, OPT_EXCLUDE_VCS},
		{"extract",              0, 0, OPT_EXTRACT},
		{"file",                 1, 0, OPT_FILE},
		{"files-from",           1, 0, OPT_FILES_FROM},
		{"format",               1, 0, OPT_FORMAT},
		{"function",             1, 0, OPT_FUNCTION},
		{"get",                  0, 0, OPT_EXTRACT},
		{"group",                1, 0, OPT_GROUP},
		{"gunzip",               0, 0, OPT_GZIP},
		{"gzip",                 0, 0, OPT_GZIP},
		{"help",                 0, 0, OPT_HELP},
		{"label",                1, 0, OPT_LABEL},
		{"list",                 0, 0, OPT_LIST},
		{"list-filters",         0, 0, OPT_LIST_FILTERS},
		{"list-formats",         0, 0, OPT_LIST_FORMATS},
		{"list-functions",       0, 0, OPT_LIST_FUNCTINOS},
		{"list-ios",             0, 0, OPT_LIST_IOS},
		{"mode",                 1, 0, OPT_MODE},
		{"no-null",              0, 0, OPT_NO_NULL},
		{"null",                 0, 0, OPT_NULL},
		{"owner",                1, 0, OPT_OWNER},
		{"plugin",               1, 0, OPT_PLUGIN},
		{"ungzip",               0, 0, OPT_GZIP},
		{"verbose",              0, 0, OPT_VERBOSE},
		{"verify",               0, 0, OPT_VERIFY},
		{"version",              0, 0, OPT_VERSION},

		{0, 0, 0, 0},
	};

	while (optind < argc) {
		int option_index;
		int c = getopt_long(argc, argv, "b:cC:f:H:jtT:vV:WxX:z?", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case OPT_ADD_FILE:
				option->files = realloc(option->files, (option->nbFiles + 1) * sizeof(char *));
				option->files[option->nbFiles] = optarg;
				option->nbFiles++;
				break;

			case OPT_ATIME_PRESERVE:
				option->atime_preserve = MTAR_OPTION_ATIME_REPLACE;
				break;

			case OPT_BLOCKING_FACTOR:
				option->block_factor = atoi(optarg);
				break;

			case OPT_BZIP2:
				option->compress_module = "bzip2";
				break;

			case OPT_COMPRESSION_LEVEL:
				option->compress_level = atoi(optarg);
				break;

			case OPT_CREATE:
				option->doWork = mtar_function_get("create");
				break;

			case OPT_DIRECTORY:
				option->working_directory = optarg;
				break;

			case OPT_EXCLUDE:
				option->excludes = realloc(option->excludes, (option->nb_excludes + 1) * sizeof(char *));
				option->excludes[option->nb_excludes] = optarg;
				option->nb_excludes++;
				break;

			case OPT_EXCLUDE_BACKUPS:
				option->exclude_option |= MTAR_EXCLUDE_OPTION_BACKUP;
				break;

			case OPT_EXCLUDE_CACHES:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", MTAR_EXCLUDE_TAG);
				break;

			case OPT_EXCLUDE_CACHES_ALL:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", MTAR_EXCLUDE_TAG_ALL);
				break;

			case OPT_EXCLUDE_CACHES_UNDER:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", MTAR_EXCLUDE_TAG_UNDER);
				break;

			case OPT_EXCLUDE_ENGINE:
				option->exclude_engine = optarg;
				break;

			case OPT_EXCLUDE_FROM:
				option->files = mtar_file_add_from_file(argv[optind++], option->files, &option->nbFiles, option);
				break;

			case OPT_EXCLUDE_TAG:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, MTAR_EXCLUDE_TAG);
				break;

			case OPT_EXCLUDE_TAG_ALL:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, MTAR_EXCLUDE_TAG_ALL);
				break;

			case OPT_EXCLUDE_TAG_UNDER:
				option->exclude_tags = mtar_exclude_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, MTAR_EXCLUDE_TAG_UNDER);
				break;

			case OPT_EXCLUDE_VCS:
				option->exclude_option |= MTAR_EXCLUDE_OPTION_VCS;
				break;

			case OPT_EXTRACT:
				option->doWork = mtar_function_get("extract");
				break;

			case OPT_FILE:
				if (option->filename) {
					mtar_verbose_printf("File is already defined (%s)\n", option->filename);
					mtar_option_show_help(*argv);
					return 2;
				}
				option->filename = optarg;
				break;

			case OPT_FILES_FROM:
				option->files = mtar_file_add_from_file(optarg, option->files, &option->nbFiles, option);
				break;

			case OPT_FORMAT:
				option->format = optarg;
				break;

			case OPT_FUNCTION:
				if (!strncmp(optarg, "help=", 5)) {
					mtar_option_show_version(*argv);
					mtar_function_showHelp(optarg + 5);
					return 1;
				} else {
					option->doWork = mtar_function_get(optarg);
				}
				break;

			case OPT_GROUP:
				option->group = optarg;
				break;

			case OPT_GZIP:
				option->compress_module = "gzip";
				break;

			case OPT_HELP:
				mtar_option_show_help(*argv);
				return 1;

			case OPT_LABEL:
				option->label = optarg;
				break;

			case OPT_LIST:
				option->doWork = mtar_function_get("list");
				break;

			case OPT_LIST_FILTERS:
				mtar_option_show_version(*argv);
				mtar_filter_show_description();
				return 1;

			case OPT_LIST_FORMATS:
				mtar_option_show_version(*argv);
				mtar_format_show_description();
				return 1;

			case OPT_LIST_FUNCTINOS:
				mtar_option_show_version(*argv);
				mtar_function_show_description();
				return 1;

			case OPT_LIST_IOS:
				mtar_option_show_version(*argv);
				mtar_io_show_description();
				return 1;

			case OPT_MODE:
				sscanf(optarg, "%o", &option->mode);
				break;

			case OPT_NO_NULL:
				option->delimiter = '\n';
				break;

			case OPT_NULL:
				option->delimiter = '\0';
				break;

			case OPT_OWNER:
				option->owner = optarg;
				break;

			case OPT_PLUGIN:
				option->plugins = realloc(option->plugins, (option->nb_plugins + 1) * sizeof(char *));
				option->plugins[option->nb_plugins] = optarg;
				option->nb_plugins++;
				break;

			case OPT_VERBOSE:
				if (option->verbose < 2)
					option->verbose++;
				break;

			case OPT_VERIFY:
				option->verify = 1;
				break;

			case OPT_VERSION:
				mtar_option_show_version(*argv);
				return 1;
		}
	}

	if (optind < argc) {
		option->nbFiles = argc - optind;
		option->files = calloc(option->nbFiles, sizeof(char *));
		for (i = 0; optind < argc; i++, optind++)
			option->files[i] = argv[optind];
	}

	return 0;
}

void mtar_option_show_help(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf("%s: modular tar (version: %s)\n", ptr, MTAR_VERSION);
	mtar_verbose_printf("  Usage: mtar [short_option] [param_short_option] [long_option] [--] [files]\n\n");

	mtar_verbose_printf("  Main operation mode:\n");
	mtar_verbose_printf("    -c, --create                   : create new archive\n");
	mtar_verbose_printf("    -t, --list                     : list files from tar archive\n");
	mtar_verbose_printf("    -x, --extract, --get           : extract new archive\n");
	mtar_verbose_printf("        --function FUNCTION *      : use FUNCTION as action\n");
	mtar_verbose_printf("        --function help=FUNCTION * : show specific help from function FUNCTION\n\n");
	mtar_verbose_printf("  where FUNCTION is one of:\n");
	mtar_function_show_description();

	mtar_verbose_printf("\n  Overwrite control:\n");
	mtar_verbose_printf("    -W, --verify : attempt to verify the archive after writing it\n\n");

	mtar_verbose_printf("  Handling of file attributes:\n");
	mtar_verbose_printf("        --atime-preserve : preserve access times on dumped files\n");
	mtar_verbose_printf("        --group=NAME     : force NAME as group for added files\n");
	mtar_verbose_printf("        --mode=CHANGES   : force (symbolic) mode CHANGES for added files\n");
	mtar_verbose_printf("        --owner=NAME     : force NAME as owner for added files\n\n");

	mtar_verbose_printf("  Device selection and switching:\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE : use ARCHIVE file or device ARCHIVE\n\n");

	mtar_verbose_printf("  Device blocking:\n");
	mtar_verbose_printf("    -b, --blocking-factor=BLOCKS : BLOCKS x 512 bytes per record\n\n");

	mtar_verbose_printf("  Archive format selection:\n");
	mtar_verbose_printf("    -H, --format=FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf("    -V, --label=TEXT    : create archive with volume name TEXT\n\n");

	mtar_verbose_printf("  where FORMAT is one of the following:\n");
	mtar_format_show_description();

	mtar_verbose_printf("\n  Compression options:\n");
	mtar_verbose_printf("    -j, --bzip2                     : filter the archive through bzip2\n");
	mtar_verbose_printf("    -z, --gzip, --gunzip, --ungzip  : filter the archive through gzip\n");
	mtar_verbose_printf("        --compression-level=LEVEL * : Set the level of compression\n");
	mtar_verbose_printf("                                      (1 <= LEVEL <= 9)\n\n");

	mtar_verbose_printf("  Local file selection:\n");
	mtar_verbose_printf("        --add-file=FILE           : add given FILE to the archive (useful if\n");
	mtar_verbose_printf("                                    its name starts with a dash)\n");
	mtar_verbose_printf("    -C, --directory=DIR           : change to directory DIR\n");
	mtar_verbose_printf("        --exclude=PATTERN         : exclude files, given as a PATTERN\n");
	mtar_verbose_printf("        --exclude-backups         : exclude backup and lock files\n");
	mtar_verbose_printf("        --exclude-caches          : exclude contents of directories containing\n");
	mtar_verbose_printf("                                    CACHEDIR.TAG, except for the tag file\n");
	mtar_verbose_printf("                                    itself\n");
	mtar_verbose_printf("        --exclude-caches-all      : exclude directories containing\n");
	mtar_verbose_printf("                                    CACHEDIR.TAG\n");
	mtar_verbose_printf("        --exclude-caches-under    : exclude everything under directories\n");
	mtar_verbose_printf("                                    containing CACHEDIR.TAG\n");
	mtar_verbose_printf("        --exclude-engine=ENGINE * : use ENGINE to exclude filename\n");
	mtar_verbose_printf("    -X, --exclude-from=FILE       : exclude patterns listed in FILE\n");
	mtar_verbose_printf("        --exclude-tag=FILE        : exclude contents of directories containing\n");
	mtar_verbose_printf("                                    FILE, except for FILE itself\n");
	mtar_verbose_printf("        --exclude-tag-all=FILE    : exclude directories containing FILE\n");
	mtar_verbose_printf("        --exclude-tag-under=FILE  : exclude everything under directories\n");
	mtar_verbose_printf("                                    containing FILE\n");
	mtar_verbose_printf("        --exclude-vcs             : exclude version control system directories\n");
	mtar_verbose_printf("    -T, --files-from=FILE         : get names to extract or create from FILE\n");
	mtar_verbose_printf("        --no-null                 : disable the effect of the previous --null\n");
	mtar_verbose_printf("                                    option\n");
	mtar_verbose_printf("        --null                    : -T or -X reads null-terminated names\n\n");

	mtar_verbose_printf("  where ENGINE is one of the following:\n");
	mtar_exclude_show_description();

	mtar_verbose_printf("\n  Informative output:\n");
	mtar_verbose_printf("    -v, --verbose : verbosely list files processed\n\n");

	mtar_verbose_printf("  Other options:\n");
	mtar_verbose_printf("    -?, --help : give this help list\n\n");

	mtar_verbose_printf("        --list-filters *   : list available io filters\n");
	mtar_verbose_printf("        --list-formats *   : list available format\n");
	mtar_verbose_printf("        --list-functions * : list available function\n");
	mtar_verbose_printf("        --list-ios *       : list available io backend\n");
	mtar_verbose_printf("        --plugin PLUGIN *  : load a plugin which will interact with a\n");
	mtar_verbose_printf("                             function\n\n");

	mtar_verbose_printf("  Parameters marked with * do not exist into gnu tar\n");
}

void mtar_option_show_version(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	mtar_verbose_printf("%s: modular tar (version: %s, build: %s %s)\n", ptr, MTAR_VERSION, __DATE__, __TIME__);
}

