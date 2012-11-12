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
*  Last modified: Mon, 12 Nov 2012 17:21:13 +0100                           *
\***************************************************************************/

// getopt_long
#include <getopt.h>
// strlen, strncmp, strrchr, strspn
#include <string.h>
// sscanf
#include <stdio.h>
// atoi, calloc, free, realloc
#include <stdlib.h>

#include <mtar.chcksum>
#include <mtar.version>

#include <mtar/file.h>
#include <mtar/verbose.h>

#include "filter.h"
#include "format.h"
#include "function.h"
#include "io.h"
#include "pattern.h"
#include "option.h"

static void mtar_option_show_full_version(void);
static void mtar_option_show_help(void);
static void mtar_option_show_version(void);


bool mtar_option_check(struct mtar_option * option) {
	if (!option->do_work) {
		mtar_verbose_printf("No function defined\n");
		return false;
	}

	if (option->owner) {
		uid_t uid = mtar_file_user2uid(option->owner);
		if (uid == (uid_t) -1) {
			mtar_verbose_printf("Invalid user (%s)\n", option->owner);
			return false;
		}
	}

	if (option->group) {
		gid_t gid = mtar_file_group2gid(option->group);
		if (gid == (gid_t) -1) {
			mtar_verbose_printf("Invalid group (%s)\n", option->group);
			return false;
		}
	}

	return true;
}

void mtar_option_show_full_version() {
	mtar_option_show_version();
	mtar_verbose_printf("  Last git commit: %s\n  SHA1 of mtar's source files: %s\n\n", MTAR_GIT_COMMIT, MTAR_SRCSUM);

	mtar_filter_show_version();
	mtar_format_show_version();
	mtar_function_show_version();
	mtar_io_show_version();
	mtar_pattern_show_version();
}

void mtar_option_free(struct mtar_option * option) {
	// main operation mode
	option->do_work = NULL;

	// overwrite control
	option->verify = false;

	// handling of file attributes
	option->group = NULL;
	option->owner = NULL;

	// device selection and switching
	option->filename = NULL;

	// archive format selection
	option->format = NULL;
	option->label = NULL;

	// compression options
	option->compress_module = NULL;

	// local file selections
	if (option->nb_files > 0) {
		unsigned int i;
		for (i = 0; i < option->nb_files; i++)
			option->files[i]->ops->free(option->files[i]);

		free(option->files);
		option->files = NULL;
		option->nb_files = 0;
	}

	option->working_directory = NULL;

	if (option->nb_excludes > 0) {
		free(option->excludes);
		option->excludes = NULL;
		option->nb_excludes = 0;
	}

	if (option->nb_exclude_tags > 0) {
		free(option->exclude_tags);
		option->exclude_tags = NULL;
		option->nb_exclude_tags = 0;
	}
}

int mtar_option_parse(struct mtar_option * option, int argc, char ** argv) {
	// main operation mode
	option->do_work = NULL;

	// overwrite control
	option->verify = false;

	// handling of file attributes
	option->atime_preserve = mtar_option_atime_none;
	option->group = NULL;
	option->mode = 0;
	option->owner = NULL;

	// device selection and switching
	option->filename = "-";
	option->tape_length = 0;
	option->multi_volume = false;

	// device blocking
	option->block_factor = 10;

	// archive format selection
	option->format = "ustar";
	option->label = NULL;

	// compression options
	option->compress_module = NULL;
	option->compress_level = 6;

	// local file selections
	option->files = NULL;
	option->nb_files = 0;
	option->working_directory = NULL;
	option->excludes = NULL;
	option->nb_excludes = 0;
	option->exclude_option = mtar_exclude_option_default;
	option->exclude_tags = NULL;
	option->nb_exclude_tags = 0;
	option->delimiter = '\n';

	char * pattern_engine = "fnmatch";
	enum mtar_pattern_option include_pattern_option = mtar_pattern_option_default_include;
	enum mtar_pattern_option exclude_pattern_option = mtar_pattern_option_default_exclude;

	// informative output
	option->verbose = 0;

	if (argc < 2) {
		mtar_option_show_help();
		return 2;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-bcCfHjJLMtTvVWxXz?");
	if (length != goodArg && strncmp(argv[1], "--", 2)) {
		mtar_verbose_printf("Invalid argument '%c'\n", argv[1][goodArg]);
		mtar_option_show_help();
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
					option->do_work = mtar_function_get("create");
					break;

				case 'C':
					option->working_directory = argv[optind++];
					break;

				case 'f':
					if (option->filename != NULL) {
						mtar_verbose_printf("File is already defined (%s)\n", option->filename);
						mtar_option_show_help();
						return 2;
					}
					if (optind >= argc) {
						mtar_verbose_printf("Argument 'f' require a parameter\n");
						mtar_option_show_help();
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

				case 'J':
					option->compress_module = "xz";
					break;

				case 'L':
					option->tape_length = atol(argv[optind++]);
					break;

				case 'M':
					option->multi_volume = 1;
					break;

				case 't':
					option->do_work = mtar_function_get("list");
					break;

				case 'T':
					option->files = mtar_pattern_add_include_from_file(option->files, &option->nb_files, pattern_engine, include_pattern_option, argv[optind++], option);
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
					option->do_work = mtar_function_get("extract");
					break;

				case 'X':
					option->excludes = mtar_pattern_add_exclude_from_file(option->excludes, &option->nb_excludes, pattern_engine, exclude_pattern_option, argv[optind++], option);
					break;

				case 'z':
					option->compress_module = "gzip";
					break;

				case '?':
					mtar_option_show_help();
					return 1;
			}
		}
	}

	enum {
		opt_blocking_factor = 'b',
		opt_bzip2           = 'j',
		opt_create          = 'c',
		opt_directory       = 'C',
		opt_exclude_from    = 'X',
		opt_extract         = 'x',
		opt_file            = 'f',
		opt_files_from      = 'T',
		opt_format          = 'H',
		opt_gzip            = 'z',
		opt_help            = '?',
		opt_label           = 'V',
		opt_list            = 't',
		opt_multi_volume    = 'M',
		opt_tape_length     = 'L',
		opt_verbose         = 'v',
		opt_verify          = 'W',
		opt_xz              = 'J',

		opt_add_file = 256,
		opt_anchored,
		opt_atime_preserve,
		opt_compression_level,
		opt_exclude,
		opt_exclude_backups,
		opt_exclude_caches,
		opt_exclude_caches_all,
		opt_exclude_caches_under,
		opt_exclude_tag,
		opt_exclude_tag_all,
		opt_exclude_tag_under,
		opt_exclude_vcs,
		opt_full_version,
		opt_function,
		opt_group,
		opt_list_filters,
		opt_list_formats,
		opt_list_functinos,
		opt_list_ios,
		opt_mode,
		opt_no_anchored,
		opt_no_null,
		opt_no_recursion,
		opt_null,
		opt_owner,
		opt_pattern_engine,
		opt_recursion,
		opt_version,
	};

	static struct option long_options[] = {
		{ "add-file",             1, 0, opt_add_file },
		{ "anchored",             0, 0, opt_anchored },
		{ "atime-preserve",       2, 0, opt_atime_preserve },
		{ "blocking-factor",      1, 0, opt_blocking_factor },
		{ "bzip2",                0, 0, opt_bzip2 },
		{ "compression-level",    1, 0, opt_compression_level },
		{ "create",               0, 0, opt_create },
		{ "directory",            1, 0, opt_directory },
		{ "exclude",              1, 0, opt_exclude },
		{ "exclude-backups",      0, 0, opt_exclude_backups },
		{ "exclude-caches",       0, 0, opt_exclude_caches },
		{ "exclude-caches-all",   0, 0, opt_exclude_caches_all },
		{ "exclude-caches-under", 0, 0, opt_exclude_caches_under },
		{ "exclude-from",         1, 0, opt_exclude_from },
		{ "exclude-tag",          1, 0, opt_exclude_tag },
		{ "exclude-tag-all",      1, 0, opt_exclude_tag_all },
		{ "exclude-tag-under",    1, 0, opt_exclude_tag_under },
		{ "exclude-vcs",          0, 0, opt_exclude_vcs },
		{ "extract",              0, 0, opt_extract },
		{ "file",                 1, 0, opt_file },
		{ "files-from",           1, 0, opt_files_from },
		{ "format",               1, 0, opt_format },
		{ "full-version",         0, 0, opt_full_version },
		{ "function",             1, 0, opt_function },
		{ "get",                  0, 0, opt_extract },
		{ "group",                1, 0, opt_group },
		{ "gunzip",               0, 0, opt_gzip },
		{ "gzip",                 0, 0, opt_gzip },
		{ "help",                 0, 0, opt_help },
		{ "label",                1, 0, opt_label },
		{ "list",                 0, 0, opt_list },
		{ "list-filters",         0, 0, opt_list_filters },
		{ "list-formats",         0, 0, opt_list_formats },
		{ "list-functions",       0, 0, opt_list_functinos },
		{ "list-ios",             0, 0, opt_list_ios },
		{ "mode",                 1, 0, opt_mode },
		{ "multi-volume",         0, 0, opt_multi_volume },
		{ "no-anchored",          0, 0, opt_no_anchored },
		{ "no-null",              0, 0, opt_no_null },
		{ "no-recursion",         0, 0, opt_no_recursion },
		{ "null",                 0, 0, opt_null },
		{ "owner",                1, 0, opt_owner },
		{ "pattern-engine",       1, 0, opt_pattern_engine },
		{ "recursion",            0, 0, opt_recursion },
		{ "tape-length",          1, 0, opt_tape_length },
		{ "ungzip",               0, 0, opt_gzip },
		{ "verbose",              0, 0, opt_verbose },
		{ "verify",               0, 0, opt_verify },
		{ "version",              0, 0, opt_version },
		{ "xz",                   0, 0, opt_xz },

		{ 0, 0, 0, 0 },
	};

	while (optind < argc) {
		if (argv[optind][0] != '-') {
			option->files = mtar_pattern_add_include(option->files, &option->nb_files, pattern_engine, argv[optind], include_pattern_option);
			optind++;
			continue;
		}

		int option_index;
		int c = getopt_long(argc, argv, "b:cC:f:H:jML:tT:vV:WxX:z?", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case opt_add_file:
				option->files = mtar_pattern_add_include(option->files, &option->nb_files, pattern_engine, optarg, include_pattern_option);
				break;

			case opt_anchored:
				exclude_pattern_option |= mtar_pattern_option_anchored;
				break;

			case opt_atime_preserve:
				option->atime_preserve = mtar_option_atime_replace;
				break;

			case opt_blocking_factor:
				option->block_factor = atoi(optarg);
				break;

			case opt_bzip2:
				option->compress_module = "bzip2";
				break;

			case opt_compression_level:
				option->compress_level = atoi(optarg);
				break;

			case opt_create:
				option->do_work = mtar_function_get("create");
				break;

			case opt_directory:
				option->working_directory = optarg;
				break;

			case opt_exclude:
				option->excludes = mtar_pattern_add_exclude(option->excludes, &option->nb_excludes, pattern_engine, optarg, exclude_pattern_option);
				break;

			case opt_exclude_backups:
				option->exclude_option |= mtar_exclude_option_backup;
				break;

			case opt_exclude_caches:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", mtar_pattern_tag);
				break;

			case opt_exclude_caches_all:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", mtar_pattern_tag_all);
				break;

			case opt_exclude_caches_under:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, "CACHEDIR.TAG", mtar_pattern_tag_under);
				break;

			case opt_exclude_from:
				//option->files = mtar_file_add_from_file(argv[optind++], option->files, &option->nbFiles, option);
				break;

			case opt_exclude_tag:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, mtar_pattern_tag);
				break;

			case opt_exclude_tag_all:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, mtar_pattern_tag_all);
				break;

			case opt_exclude_tag_under:
				option->exclude_tags = mtar_pattern_add_tag(option->exclude_tags, &option->nb_exclude_tags, optarg, mtar_pattern_tag_under);
				break;

			case opt_exclude_vcs:
				option->exclude_option |= mtar_exclude_option_vcs;
				break;

			case opt_extract:
				option->do_work = mtar_function_get("extract");
				break;

			case opt_file:
				if (option->filename != NULL) {
					mtar_verbose_printf("File is already defined (%s)\n", option->filename);
					mtar_option_show_help();
					return 2;
				}
				option->filename = optarg;
				break;

			case opt_files_from:
				option->files = mtar_pattern_add_include_from_file(option->files, &option->nb_files, pattern_engine, include_pattern_option, optarg, option);
				break;

			case opt_format:
				option->format = optarg;
				break;

			case opt_full_version:
				mtar_option_show_full_version();
				return 1;

			case opt_function:
				if (!strncmp(optarg, "help=", 5)) {
					mtar_option_show_version();
					mtar_function_show_help(optarg + 5);
					return 1;
				} else {
					option->do_work = mtar_function_get(optarg);
				}
				break;

			case opt_group:
				option->group = optarg;
				break;

			case opt_gzip:
				option->compress_module = "gzip";
				break;

			case opt_help:
				mtar_option_show_help();
				return 1;

			case opt_label:
				option->label = optarg;
				break;

			case opt_list:
				option->do_work = mtar_function_get("list");
				break;

			case opt_list_filters:
				mtar_option_show_version();
				mtar_filter_show_description();
				return 1;

			case opt_list_formats:
				mtar_option_show_version();
				mtar_format_show_description();
				return 1;

			case opt_list_functinos:
				mtar_option_show_version();
				mtar_function_show_description();
				return 1;

			case opt_list_ios:
				mtar_option_show_version();
				mtar_io_show_description();
				return 1;

			case opt_mode:
				sscanf(optarg, "%o", &option->mode);
				break;

			case opt_multi_volume:
				option->multi_volume = 1;
				break;

			case opt_no_null:
				option->delimiter = '\n';
				break;

			case opt_no_anchored:
				exclude_pattern_option &= ~mtar_pattern_option_anchored;
				break;

			case opt_no_recursion:
				exclude_pattern_option &= ~mtar_pattern_option_recursion;
				include_pattern_option &= ~mtar_pattern_option_recursion;
				break;

			case opt_null:
				option->delimiter = '\0';
				break;

			case opt_owner:
				option->owner = optarg;
				break;

			case opt_pattern_engine:
				pattern_engine = optarg;
				break;

			case opt_recursion:
				exclude_pattern_option |= mtar_pattern_option_recursion;
				include_pattern_option |= mtar_pattern_option_recursion;
				break;

			case opt_tape_length:
				sscanf(optarg, "%zd", &option->tape_length);
				break;

			case opt_verbose:
				if (option->verbose < 2)
					option->verbose++;
				break;

			case opt_verify:
				option->verify = 1;
				break;

			case opt_version:
				mtar_option_show_version();
				return 1;

			case opt_xz:
				option->compress_module = "xz";
				break;
		}
	}

	while (optind < argc)
		option->files = mtar_pattern_add_include(option->files, &option->nb_files, pattern_engine, argv[optind++], include_pattern_option);

	return 0;
}

void mtar_option_show_help() {
	mtar_verbose_printf("mtar: modular tar (version: %s)\n", MTAR_VERSION);
	mtar_verbose_printf("Usage: mtar [short_option] [param_short_option] [long_option] [--] [files]\n\n");

	mtar_verbose_printf("  Main operation mode:\n");
	mtar_verbose_print_help("-c, --create : create new archive");
	mtar_verbose_print_help("-t, --list : list files from tar archive");
	mtar_verbose_print_help("-x, --extract, --get : extract new archive");
	mtar_verbose_print_help("--function FUNCTION * : use FUNCTION as action");
	mtar_verbose_print_help("--function help=FUNCTION * : show specific help from function FUNCTION");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Where FUNCTION is one of:\n");
	mtar_function_show_description();
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Overwrite control:\n");
	mtar_verbose_print_help("-W, --verify : attempt to verify the archive after writing it");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Handling of file attributes:\n");
	mtar_verbose_print_help("--atime-preserve : preserve access times on dumped files");
	mtar_verbose_print_help("--group=NAME : force NAME as group for added files");
	mtar_verbose_print_help("--mode=CHANGES : force (symbolic) mode CHANGES for added files");
	mtar_verbose_print_help("--owner=NAME : force NAME as owner for added files");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Device selection and switching:\n");
	mtar_verbose_print_help("-f, --file=ARCHIVE : use ARCHIVE file or device ARCHIVE");
	mtar_verbose_print_help("-L, --tape-length=NUMBER : change tape after writing NUMBER x 1024 bytes");
	mtar_verbose_print_help("-M, --multi-volume : create/list/extract multi-volume archive");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Device blocking:\n");
	mtar_verbose_print_help("-b, --blocking-factor=BLOCKS : BLOCKS x 512 bytes per record");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Archive format selection:\n");
	mtar_verbose_print_help("-H, --format=FORMAT : use FORMAT as tar format");
	mtar_verbose_print_help("-V, --label=TEXT : create archive with volume name TEXT");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Where FORMAT is one of the following:\n");
	mtar_format_show_description();
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Compression options:\n");
	mtar_verbose_print_help("-j, --bzip2 : filter the archive through bzip2");
	mtar_verbose_print_help("-J, --xz : filter the archive through xz");
	mtar_verbose_print_help("-z, --gzip, --gunzip, --ungzip : filter the archive through gzip");
	mtar_verbose_print_help("--compression-level=LEVEL * : Set the level of compression (1 <= LEVEL <= 9)");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Local file selection:\n");
	mtar_verbose_print_help("--add-file=FILE : add given FILE to the archive (useful if its name starts with a dash)");
	mtar_verbose_print_help("-C, --directory=DIR : change to directory DIR");
	mtar_verbose_print_help("--exclude=PATTERN : exclude files, given as a PATTERN");
	mtar_verbose_print_help("--exclude-backups : exclude backup and lock files");
	mtar_verbose_print_help("--exclude-caches : exclude contents of directories containing CACHEDIR.TAG, except for the tag file itself");
	mtar_verbose_print_help("--exclude-caches-all : exclude directories containing CACHEDIR.TAG");
	mtar_verbose_print_help("--exclude-caches-under : exclude everything under directories containing CACHEDIR.TAG");
	mtar_verbose_print_help("-X, --exclude-from=FILE : exclude patterns listed in FILE");
	mtar_verbose_print_help("--exclude-tag=FILE : exclude contents of directories containing FILE, except for FILE itself");
	mtar_verbose_print_help("--exclude-tag-all=FILE : exclude directories containing FILE");
	mtar_verbose_print_help("--exclude-tag-under=FILE : exclude everything under directories containing FILE");
	mtar_verbose_print_help("--exclude-vcs : exclude version control system directories");
	mtar_verbose_print_help("-T, --files-from=FILE : get names to extract or create from FILE");
	mtar_verbose_print_help("--no-null : disable the effect of the previous --null option");
	mtar_verbose_print_help("--no-recursion : avoid descending automatically in directories");
	mtar_verbose_print_help("--null : -T or -X reads null-terminated names");
	mtar_verbose_print_help("--pattern-engine=ENGINE * : use ENGINE to match filename");
	mtar_verbose_print_help("--recursion : recurse into directories (default)");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Where ENGINE is one of the following:\n");
	mtar_pattern_show_description();
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Informative output:\n");
	mtar_verbose_print_help("-v, --verbose : verbosely list files processed");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("  Other options:\n");
	mtar_verbose_print_help("-?, --help : give this help list");

	mtar_verbose_print_help("--list-filters * : list available io filters");
	mtar_verbose_print_help("--list-formats * : list available format");
	mtar_verbose_print_help("--list-functions * : list available function");
	mtar_verbose_print_help("--list-ios * : list available io backend");
	mtar_verbose_print_flush(4, 1);

	mtar_verbose_printf("Parameters marked with * do not exist into gnu tar\n");
}

void mtar_option_show_version() {
	mtar_verbose_printf("mtar: modular tar (version: " MTAR_VERSION ", build: " __DATE__ ", " __TIME__ ")\n");
}

