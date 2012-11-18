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
*  Last modified: Sun, 18 Nov 2012 11:44:48 +0100                           *
\***************************************************************************/

#ifndef __MTAR_VERBOSE_H__
#define __MTAR_VERBOSE_H__

// bool
#include <stdbool.h>

void mtar_verbose_clean(void);
void mtar_verbose_flush(void);
void mtar_verbose_print_flush(int tab_level, bool new_line);
void mtar_verbose_print_help(const char * format, ...) __attribute__ ((format (printf, 1, 2)));
void mtar_verbose_printf(const char * format, ...) __attribute__ ((format (printf, 1, 2)));
void mtar_verbose_progress(const char * format, unsigned long long current, unsigned long long upperLimit);
char * mtar_verbose_prompt(const char * format, ...) __attribute__ ((format (printf, 1, 2)));
void mtar_verbose_restart_timer(void);
void mtar_verbose_stop_timer(void);

#endif

