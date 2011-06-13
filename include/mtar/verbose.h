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
*  Last modified: Wed, 08 Jun 2011 08:37:59 +0200                       *
\***********************************************************************/

#ifndef __MTAR_VERBOSE_H__
#define __MTAR_VERBOSE_H__

enum mtar_verbose_level {
	MTAR_VERBOSE_LEVEL_DEBUG   = 0x3,
	MTAR_VERBOSE_LEVEL_ERROR   = 0x0,
	MTAR_VERBOSE_LEVEL_INFO    = 0x2,
	MTAR_VERBOSE_LEVEL_WARNING = 0x1,
};

void mtar_verbose_clean(void);
void mtar_verbose_printf(enum mtar_verbose_level level, const char * format, ...) __attribute__ ((format (printf, 2, 3)));
void mtar_verbose_progress(const char * format, unsigned long long current, unsigned long long upperLimit);
void mtar_verbose_restart_timer(void);
void mtar_verbose_stop_timer(void);

#endif

