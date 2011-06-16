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
*  Last modified: Thu, 16 Jun 2011 09:57:55 +0200                       *
\***********************************************************************/

#include <mtar/verbose.h>

#include "common.h"

static void mtar_format_showDescription(void);

static struct mtar_format mtar_format_ustar = {
	.name            = "ustar",
	.newIn           = mtar_format_ustar_newIn,
	.newOut          = mtar_format_ustar_newOut,
	.showDescription = mtar_format_showDescription,
};

__attribute__((constructor))
static void format_init() {
	mtar_format_register(&mtar_format_ustar);
}

void mtar_format_showDescription() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  ustar : default format in gnu tar\n");
}

