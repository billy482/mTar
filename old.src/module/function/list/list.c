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
*  Last modified: Mon, 31 Oct 2011 15:25:19 +0100                           *
\***************************************************************************/

#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

#include "common.h"

static int mtar_function_list(const struct mtar_option * option);
static void mtar_function_list_init(void) __attribute__((constructor));
static void mtar_function_list_show_description(void);
static void mtar_function_list_show_help(void);

static struct mtar_function mtar_function_list_functions = {
	.name             = "list",
	.doWork           = mtar_function_list,
	.show_description = mtar_function_list_show_description,
	.show_help        = mtar_function_list_show_help,
	.api_version      = MTAR_FUNCTION_API_VERSION,
};


int mtar_function_list(const struct mtar_option * option) {
	struct mtar_format_in * format = mtar_format_get_in(option);
	if (!format)
		return 1;

	mtar_function_list_configure(option);
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

				mtar_function_list_display(&header);
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
