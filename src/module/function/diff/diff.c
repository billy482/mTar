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
*  Last modified: Fri, 16 Nov 2012 16:24:16 +0100                           *
\***************************************************************************/

// fstatat, openat
#include <fcntl.h>
// free
#include <stdlib.h>
// fstatat
#include <sys/stat.h>
// waitpid
#include <sys/types.h>
// waitpid
#include <sys/wait.h>
// execl, _exit, fork
#include <unistd.h>

#include <mtar-function-diff.chcksum>
#include <mtar.version>

#include <mtar/filter.h>
#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

#include "diff.h"

struct mtar_function_diff_param {
	struct mtar_format_reader * format;

	const char * filename;
	unsigned int i_volume;

	const struct mtar_option * option;
};

static int mtar_function_diff(const struct mtar_option * option);
static void mtar_function_diff_init(void) __attribute__((constructor));
static int mtar_function_diff_select_volume(struct mtar_function_diff_param * param);
static void mtar_function_diff_show_description(void);
static void mtar_function_diff_show_help(void);
static void mtar_function_diff_show_version(void);

static struct mtar_function mtar_function_diff_functions = {
	.name             = "diff",

	.do_work           = mtar_function_diff,

	.show_description = mtar_function_diff_show_description,
	.show_help        = mtar_function_diff_show_help,
	.show_version     = mtar_function_diff_show_version,

	.api_level        = {
		.filter   = 0,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = MTAR_FUNCTION_API_LEVEL,
		.io       = MTAR_IO_API_LEVEL,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = MTAR_PATTERN_API_LEVEL,
	},
};


static int mtar_function_diff(const struct mtar_option * option) {
	struct mtar_function_diff_param param = {
		.format = mtar_format_get_reader(option),

		.filename = option->filename,
		.i_volume = 1,

		.option = option,
	};

	if (param.format == NULL)
		return 1;

	mtar_function_diff_configure(option);
	struct mtar_format_header header;

	int dir_fd = AT_FDCWD;
	if (option->working_directory != NULL) {
		dir_fd = openat(AT_FDCWD, option->working_directory, O_RDONLY | O_NOCTTY | O_NONBLOCK | O_DIRECTORY | O_CLOEXEC);
		if (dir_fd == -1) {
			mtar_verbose_printf("Fatal error: failed to change directory (%s)\n", option->working_directory);
			return 1;
		}
	}

	int ok = -1, failed;
	struct stat st;
	while (ok < 0) {
		enum mtar_format_reader_header_status status = param.format->ops->get_header(param.format, &header);

		bool display = option->nb_files == 0;
		unsigned int i;
		switch (status) {
			case mtar_format_header_ok:
				for (i = 0; !display && i < option->nb_files; i++)
					display = option->files[i]->ops->match(option->files[i], header.path);

				if (display && !mtar_pattern_match(option, header.path))
					mtar_function_diff_display(&header);

				failed = fstatat(dir_fd, header.path, &st, AT_SYMLINK_NOFOLLOW);
				if (!failed) {
					if ((st.st_mode & 07777) != (header.mode & 07777))
						mtar_verbose_printf("%s: permission differs\n", header.path);

					if (st.st_mtime != header.mtime)
						mtar_verbose_printf("%s: Mod time differs\n", header.path);

					if ((st.st_mode & S_IFMT) != (header.mode & S_IFMT)) {
						mtar_verbose_printf("%s: file type differs\n", header.path);
					} else if (S_ISREG(st.st_mode)) {
						if (st.st_size != header.size)
							mtar_verbose_printf("%s: : Size differs\n", header.path);
					}
				} else {
					mtar_verbose_printf("There is no file named %s\n", header.path);
				}

				break;

			case mtar_format_header_bad_checksum:
				mtar_verbose_printf("Bad checksum\n");
				ok = 3;
				continue;

			case mtar_format_header_bad_header:
				mtar_verbose_printf("Bad header\n");
				ok = 4;
				continue;

			case mtar_format_header_end_of_tape:
				ok = mtar_function_diff_select_volume(&param);
				continue;

			case mtar_format_header_error:
				mtar_verbose_printf("Error while reader header\n");
				ok = 5;
				continue;

			case mtar_format_header_not_found:
				ok = 0;
				continue;
		}

		mtar_format_free_header(&header);

		status = param.format->ops->skip_file(param.format);
		switch (status) {
			case mtar_format_header_end_of_tape:
				ok = mtar_function_diff_select_volume(&param);
				continue;

			case mtar_format_header_ok:
				break;

			default:
				mtar_verbose_printf("Failed to skip file\n");
				ok = 2;
		}
	}

	close(dir_fd);
	param.format->ops->free(param.format);

	return 0;
}

static void mtar_function_diff_init() {
	mtar_function_register(&mtar_function_diff_functions);
}

static int mtar_function_diff_select_volume(struct mtar_function_diff_param * param) {
	for (;;) {
		char * line = mtar_verbose_prompt("Select volume #%u for `%s' and hit return: ", param->i_volume + 1, param->filename);
		if (!line)
			break;

		switch (*line) {
			case 'n': {
				// new filename
					char * filename;
					for (filename = line + 1; *filename == ' '; filename++);

					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(filename, param->option);

					if (next_reader != NULL && param->format != NULL) {
						param->format->ops->next_volume(param->format, next_reader);
						param->i_volume++;
						free(line);
						return 0;
					} else if (next_reader != NULL) {
						param->format = mtar_format_get_reader2(next_reader, param->option);
						param->i_volume++;
						free(line);
						return 0;
					}
				}
				return 0;

			case 'q':
				mtar_verbose_printf("Archive is not fully parsed\n");
				_exit(1);

			case '\0':
			case 'y': {
					// reuse same filename
					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(param->filename, param->option);

					if (next_reader != NULL && param->format != NULL) {
						param->format->ops->next_volume(param->format, next_reader);
						param->i_volume++;
						free(line);
						return 0;
					} else if (next_reader != NULL) {
						param->format = mtar_format_get_reader2(next_reader, param->option);
						param->i_volume++;
						free(line);
						return 0;
					}
				}
				break;

			case '!': {
					// spawn shell
					pid_t pid = fork();
					if (pid > 0) {
						int status;
						waitpid(pid, &status, 0);
					} else if (pid == 0) {
						int failed = execl("/bin/bash", "bash", (const char *) NULL);
						_exit(failed);
					} else {
						mtar_verbose_printf("Failed to spawn shell because %m\n");
					}
				}
				break;

			case '?':
				mtar_verbose_printf(" n name        Give a new file name for the next (and subsequent) volume(s)\n");
				mtar_verbose_printf(" q             Abort mtar\n");
				mtar_verbose_printf(" y or newline  Continue operation\n");
				mtar_verbose_printf(" !             Spawn a subshell\n");
				mtar_verbose_printf(" ?             Print this list\n");
				break;
		}

		free(line);
	}

	return 1;
}

static void mtar_function_diff_show_description() {
	mtar_verbose_print_help("diff : find differences between archive and file system");
}

static void mtar_function_diff_show_help() {
	mtar_verbose_printf("  Find differences between archive and file system\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf("    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf("    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf("    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf("    -v, --verbose       : verbosely list files processed\n");
}

static void mtar_function_diff_show_version() {
	mtar_verbose_printf("  diff: find differences between archive and file system (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("        SHA1 of source files: " MTAR_FUNCTION_DIFF_SRCSUM "\n");
}

