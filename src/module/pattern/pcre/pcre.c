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
*  Last modified: Sun, 30 Oct 2011 23:31:22 +0100                           *
\***************************************************************************/

// pcre_version
#include <pcre.h>

#include <mtar/verbose.h>

#include "common.h"

static void mtar_pattern_pcre_init(void) __attribute__((constructor));
static void mtar_pattern_pcre_show_description(void);

static struct mtar_pattern_driver mtar_pattern_pcre_driver = {
	.name             = "pcre",
	.new_exclude      = mtar_pattern_pcre_new_exclude,
	.new_include      = mtar_pattern_pcre_new_include,
	.show_description = mtar_pattern_pcre_show_description,
	.api_version      = MTAR_PATTERN_API_VERSION,
};


void mtar_pattern_pcre_init() {
	mtar_pattern_register(&mtar_pattern_pcre_driver);
}

void mtar_pattern_pcre_show_description() {
	mtar_verbose_print_help(2, "pcre : pcre powered based pattern matching (using libpcre: v%s)\n", pcre_version());
}

