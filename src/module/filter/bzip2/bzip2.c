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
*  Last modified: Thu, 17 May 2012 12:41:05 +0200                           *
\***************************************************************************/

// BZ2_bzlibVersion
#include <bzlib.h>

#include <mtar-filter-bzip2.chcksum>
#include <mtar.version>

#include <mtar/verbose.h>

#include "bzip2.h"

static void mtar_filter_bzip2_init(void) __attribute__((constructor));
static void mtar_filter_bzip2_show_description(void);
static void mtar_filter_bzip2_show_version(void);

static struct mtar_filter mtar_filter_bzip2 = {
	.name             = "bzip2",

	.new_in           = mtar_filter_bzip2_new_in,
	.new_out          = mtar_filter_bzip2_new_out,

	.show_description = mtar_filter_bzip2_show_description,
	.show_version     = mtar_filter_bzip2_show_version,

	.api_version      = MTAR_FILTER_API_VERSION,
};


void mtar_filter_bzip2_init() {
	mtar_filter_register(&mtar_filter_bzip2);
}

void mtar_filter_bzip2_show_description() {
	mtar_verbose_print_help("bzip2 : filter from/to compressed data (using libbz2: v%s)", BZ2_bzlibVersion());
}

void mtar_filter_bzip2_show_version() {
	mtar_verbose_printf("  bzip2 : filter from/to compressed data (version: " MTAR_VERSION ") (using libbz2: v%s)\n", BZ2_bzlibVersion());
	mtar_verbose_printf("          SHA1 of source files: " MTAR_FILTER_BZIP2_SRCSUM "\n");
}

