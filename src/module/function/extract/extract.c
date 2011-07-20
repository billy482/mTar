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
*  Last modified: Wed, 20 Jul 2011 20:36:22 +0200                       *
\***********************************************************************/

// mknod, open
#include <fcntl.h>
// mkdir, mknod, open
#include <sys/stat.h>
// mkdir, mknod, open
#include <sys/types.h>
// chdir, close, link, mknod, symlink, write
#include <unistd.h>

#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "common.h"

static int mtar_function_extract(const struct mtar_option * option);
static void mtar_function_extract_init(void) __attribute__((constructor));
static void mtar_function_extract_show_description(void);
static void mtar_function_extract_show_help(void);

static struct mtar_function mtar_function_extract_functions = {
	.name            = "extract",
	.doWork          = mtar_function_extract,
	.showDescription = mtar_function_extract_show_description,
	.showHelp        = mtar_function_extract_show_help,
};


int mtar_function_extract(const struct mtar_option * option) {
	struct mtar_format_in * format = mtar_format_get_in(option);
	if (!format)
		return 1;

	mtar_function_extract_configure(option);

	if (option->working_directory && chdir(option->working_directory)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Fatal error: failed to change directory (%s)\n", option->working_directory);
		return 1;
	}

	struct mtar_format_header header;

	int ok = -1;
	while (ok < 0) {
		enum mtar_format_in_header_status status = format->ops->get_header(format, &header);

		switch (status) {
			case MTAR_FORMAT_HEADER_OK:
				mtar_function_extract_display(&header);

				if (header.is_label)
					break;

				if (header.link[0] != '\0' && !(header.mode & S_IFMT)) {
					link(header.link, header.path);
				} else if (S_ISFIFO(header.mode)) {
					mknod(header.path, S_IFIFO, 0);
				} else if (S_ISCHR(header.mode)) {
					mknod(header.path, S_IFCHR, header.dev);
				} else if (S_ISDIR(header.mode)) {
					mkdir(header.path, header.mode);
				} else if (S_ISBLK(header.mode)) {
					mknod(header.path, S_IFBLK, header.dev);
				} else if (S_ISREG(header.mode)) {
					int fd = open(header.path, O_CREAT | O_TRUNC | O_WRONLY, header.mode);

					char buffer[4096];
					ssize_t nbRead;
					while ((nbRead = format->ops->read(format, buffer, 4096)) > 0)
						write(fd, buffer, nbRead);
					close(fd);
				} else if (S_ISLNK(header.mode)) {
					symlink(header.link, header.path);
				}

				break;

			case MTAR_FORMAT_HEADER_BAD_CHECKSUM:
				mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Bad checksum\n");
				ok = 3;
				continue;

			case MTAR_FORMAT_HEADER_BAD_HEADER:
				mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Bad header\n");
				ok = 4;
				continue;

			case MTAR_FORMAT_HEADER_NOT_FOUND:
				ok = 0;
				continue;
		}
	}

	format->ops->free(format);

	return 0;
}

void mtar_function_extract_init() {
	mtar_function_register(&mtar_function_extract_functions);
}

void mtar_function_extract_show_description() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    extract : Extract files from tar archive\n");
}

void mtar_function_extract_show_help() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  List files from tar archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -C, --directory=DIR : change to directory DIR\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -v, --verbose       : verbosely extract files processed\n");
}

