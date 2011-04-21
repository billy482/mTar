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
*  Last modified: Wed, 20 Apr 2011 22:50:31 +0200                       *
\***********************************************************************/

#ifndef __MTAR_VERBOSE_H__
#define __MTAR_VERBOSE_H__

#include "common.h"

struct mtar_verbose {
	void (*clean)() __attribute__((deprecated));
	void (*print)(const char * format, ...) __attribute__ ((format (printf, 1, 2))) __attribute__((deprecated));
};

void mtar_verbose_clean(void);
void mtar_verbose_get(struct mtar_verbose * verbose, struct mtar_option * option) __attribute__((deprecated));
void mtar_verbose_printf(enum mtar_verbose_level level, const char * format, ...) __attribute__ ((format (printf, 2, 3)));

#endif

