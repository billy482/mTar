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
*  Last modified: Sat, 20 Oct 2012 13:57:11 +0200                           *
\***************************************************************************/

#include <mtar-function-list.chcksum>
#include <mtar.version>

#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

#include "list.h"

static int mtar_function_list(const struct mtar_option * option);
static void mtar_function_list_init(void) __attribute__((constructor));
static void mtar_function_list_show_description(void);
static void mtar_function_list_show_help(void);
static void mtar_function_list_show_version(void);

static struct mtar_function mtar_function_list_functions = {
	.name             = "list",

	.do_work           = mtar_function_list,

	.show_description = mtar_function_list_show_description,
	.show_help        = mtar_function_list_show_help,
	.show_version     = mtar_function_list_show_version,

	.api_level        = {
		.filter   = 0,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = MTAR_FUNCTION_API_LEVEL,
		.io       = MTAR_IO_API_LEVEL,
		.pattern  = MTAR_PATTERN_API_LEVEL,
	},
};


int mtar_function_list(const struct mtar_option * option) {
	struct mtar_format_reader * format = mtar_format_get_reader(option);
	if (!format)
		return 1;

	mtar_function_list_configure(option);
	struct mtar_format_header header;

	int ok = -1;
	while (ok < 0) {
		enum mtar_format_reader_header_status status = format->ops->get_header(format, &header);

		switch (status) {
			case mtar_format_header_ok:
				if (mtar_pattern_match(option, header.path)) {
					format->ops->skip_file(format);
					continue;
				}

				mtar_function_list_display(&header);
				break;

			case mtar_format_header_bad_checksum:
				mtar_verbose_printf("Bad checksum\n");
				ok = 3;
				continue;

			case mtar_format_header_bad_header:
				mtar_verbose_printf("Bad header\n");
				ok = 4;
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

		if (format->ops->skip_file(format)) {
			mtar_verbose_printf("Failed to skip file\n");
			ok = 2;
		}
	}

	format->ops->free(format);

	return 0;
}

void mtar_function_list_init() {
	mtar_function_register(&mtar_function_list_functions);
}

void mtar_function_list_show_description() {
	mtar_verbose_print_help("list : List files from tar archive");
}

void mtar_function_list_show_help() {
	mtar_verbose_printf("  List files from tar archive\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf("    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf("    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf("    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf("    -v, --verbose       : verbosely list files processed\n");
}

void mtar_function_list_show_version() {
	mtar_verbose_printf("  list: List files from tar archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("        SHA1 of source files: " MTAR_FUNCTION_LIST_SRCSUM "\n");
}

