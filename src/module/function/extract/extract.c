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
*  Last modified: Sun, 20 May 2012 12:36:45 +0200                           *
\***************************************************************************/

// mknod, open
#include <fcntl.h>
// mkdir, mknod, open
#include <sys/stat.h>
// mkdir, mknod, open
#include <sys/types.h>
// chdir, close, link, mknod, symlink, write
#include <unistd.h>

#include <mtar-function-extract.chcksum>
#include <mtar.version>

#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

#include "extract.h"

static int mtar_function_extract(const struct mtar_option * option);
static void mtar_function_extract_init(void) __attribute__((constructor));
static void mtar_function_extract_show_description(void);
static void mtar_function_extract_show_help(void);
static void mtar_function_extract_show_version(void);

static struct mtar_function mtar_function_extract_functions = {
	.name             = "extract",

	.do_work           = mtar_function_extract,

	.show_description = mtar_function_extract_show_description,
	.show_help        = mtar_function_extract_show_help,
	.show_version     = mtar_function_extract_show_version,

	.api_version      = MTAR_FUNCTION_API_VERSION,
};


int mtar_function_extract(const struct mtar_option * option) {
	struct mtar_format_in * format = mtar_format_get_in(option);
	if (!format)
		return 1;

	mtar_function_extract_configure(option);

	if (option->working_directory && chdir(option->working_directory)) {
		mtar_verbose_printf("Fatal error: failed to change directory (%s)\n", option->working_directory);
		return 1;
	}

	struct mtar_format_header header;

	int ok = -1;
	while (ok < 0) {
		enum mtar_format_in_header_status status = format->ops->get_header(format, &header);

		switch (status) {
			case MTAR_FORMAT_HEADER_OK:
				if (mtar_pattern_match(option, header.path)) {
					format->ops->skip_file(format);
					continue;
				}

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
					ssize_t nbRead, nbTotalRead = 0;
					while ((nbRead = format->ops->read(format, buffer, 4096)) > 0) {
						write(fd, buffer, nbRead);

						nbTotalRead += nbRead;

						mtar_function_extract_progress(header.path, "\r%b [%P] ETA: %E", nbTotalRead, header.size);
					}
					mtar_verbose_clean();
					close(fd);

				} else if (S_ISLNK(header.mode)) {
					symlink(header.link, header.path);
				}

				break;

			case MTAR_FORMAT_HEADER_BAD_CHECKSUM:
				mtar_verbose_printf("Bad checksum\n");
				ok = 3;
				continue;

			case MTAR_FORMAT_HEADER_BAD_HEADER:
				mtar_verbose_printf("Bad header\n");
				ok = 4;
				continue;

			case MTAR_FORMAT_HEADER_NOT_FOUND:
				ok = 0;
				continue;
		}

		mtar_format_free_header(&header);
	}

	format->ops->free(format);

	return 0;
}

void mtar_function_extract_init() {
	mtar_function_register(&mtar_function_extract_functions);
}

void mtar_function_extract_show_description() {
	mtar_verbose_print_help("extract : Extract files from tar archive");
}

void mtar_function_extract_show_help() {
	mtar_verbose_printf("  List files from tar archive\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf("    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf("    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf("    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf("    -C, --directory=DIR : change to directory DIR\n");
	mtar_verbose_printf("    -v, --verbose       : verbosely extract files processed\n");
}

void mtar_function_extract_show_version() {
	mtar_verbose_printf("  extract : Extract files from tar archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("            SHA1 of source files: " MTAR_FUNCTION_EXTRACT_SRCSUM "\n");
}

