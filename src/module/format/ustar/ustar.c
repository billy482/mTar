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
*  Last modified: Thu, 15 Nov 2012 13:52:54 +0100                           *
\***************************************************************************/

#include <mtar-format-ustar.chcksum>
#include <mtar.version>

#include <mtar/verbose.h>

#include "ustar.h"

static void mtar_format_ustar_format_init(void) __attribute__((constructor));
static void mtar_format_ustar_show_description(void);
static void mtar_format_ustar_show_version(void);

static struct mtar_format mtar_format_ustar = {
	.name             = "ustar",

	.auto_detect      = mtar_format_ustar_auto_detect,
	.new_reader       = mtar_format_ustar_new_reader,
	.new_writer       = mtar_format_ustar_new_writer,

	.show_description = mtar_format_ustar_show_description,
	.show_version     = mtar_format_ustar_show_version,

	.api_level        = {
		.filter   = 0,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = 0,
		.io       = 0,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = 0,
	},
};


static void mtar_format_ustar_format_init() {
	mtar_format_register(&mtar_format_ustar);
}

static void mtar_format_ustar_show_description() {
	mtar_verbose_print_help("ustar : default format in gnu tar");
}

static void mtar_format_ustar_show_version() {
	mtar_verbose_printf("  ustar: default format of mtar (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("         SHA1 of source files: " MTAR_FORMAT_USTAR_SRCSUM "\n");
}

