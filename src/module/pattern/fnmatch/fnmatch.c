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
*  Last modified: Sun, 13 May 2012 00:41:42 +0200                           *
\***************************************************************************/

#include <mtar-pattern-fnmatch.chcksum>

#include <mtar/verbose.h>

#include "fnmatch.h"

static void mtar_pattern_fnmatch_init(void) __attribute__((constructor));
static void mtar_pattern_fnmatch_show_description(void);
static void mtar_pattern_fnmatch_show_version(void);

static struct mtar_pattern_driver mtar_pattern_fnmatch_driver = {
	.name             = "fnmatch",

	.new_exclude      = mtar_pattern_fnmatch_new_exclude,
	.new_include      = mtar_pattern_fnmatch_new_include,

	.show_description = mtar_pattern_fnmatch_show_description,
	.show_version     = mtar_pattern_fnmatch_show_version,

	.api_version      = MTAR_PATTERN_API_VERSION,
};


void mtar_pattern_fnmatch_init() {
	mtar_pattern_register(&mtar_pattern_fnmatch_driver);
}

void mtar_pattern_fnmatch_show_description() {
	mtar_verbose_print_help("fnmatch : fnmatch based pattern matching");
}

void mtar_pattern_fnmatch_show_version() {
	mtar_verbose_printf("  fnmatch : fnmatch based pattern matching (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("            SHA1 of source files: " MTAR_PATTERN_FNMATCH_SRCSUM "\n");
}

