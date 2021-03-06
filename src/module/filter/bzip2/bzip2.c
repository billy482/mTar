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
*  Last modified: Sun, 18 Nov 2012 11:49:55 +0100                           *
\***************************************************************************/

// BZ2_bzlibVersion
#include <bzlib.h>

#include <mtar-filter-bzip2.chcksum>
#include <mtar.version>

#include <mtar/verbose.h>

#include "bzip2.h"

static void mtar_filter_bzip2_init(void) __attribute__((constructor));
static void mtar_filter_bzip2_show_description(void);
static void mtar_filter_bzip2_show_help(void);
static void mtar_filter_bzip2_show_version(void);

static struct mtar_filter mtar_filter_bzip2 = {
	.name             = "bzip2",

	.new_reader       = mtar_filter_bzip2_new_reader,
	.new_writer       = mtar_filter_bzip2_new_writer,

	.show_description = mtar_filter_bzip2_show_description,
	.show_help        = mtar_filter_bzip2_show_help,
	.show_version     = mtar_filter_bzip2_show_version,

	.api_level        = {
		.filter   = MTAR_FILTER_API_LEVEL,
		.format   = 0,
		.function = 0,
		.io       = 0,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = 0,
	},
};


static void mtar_filter_bzip2_init() {
	mtar_filter_register(&mtar_filter_bzip2);
}

static void mtar_filter_bzip2_show_description() {
	mtar_verbose_print_help("bzip2 : filter from/to compressed data (using libbz2: v%s)", BZ2_bzlibVersion());
}

static void mtar_filter_bzip2_show_help() {
	mtar_verbose_printf("  bzip2: filter from/to compressed data (version: " MTAR_VERSION ") (using libbz2: v%s)\n", BZ2_bzlibVersion());

	mtar_verbose_printf("    Reader:\n");
	mtar_verbose_printf("      No option available\n");

	mtar_verbose_printf("    Writer: \n");
	mtar_verbose_print_help("compression-level: an integer between 0 and 9");
	mtar_verbose_print_flush(6, false);
}

static void mtar_filter_bzip2_show_version() {
	mtar_verbose_printf("  bzip2: filter from/to compressed data (version: " MTAR_VERSION ") (using libbz2: v%s)\n", BZ2_bzlibVersion());
	mtar_verbose_printf("         SHA1 of source files: " MTAR_FILTER_BZIP2_SRCSUM "\n");
}

