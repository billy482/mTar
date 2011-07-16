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
*  Last modified: Sat, 16 Jul 2011 19:29:26 +0200                       *
\***********************************************************************/

// localtime_r, strftime
#include <time.h>
// strcat, strcpy
#include <string.h>
// S_*
#include <sys/stat.h>

#include <mtar/file.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "common.h"

static void mtar_function_list_display1(struct mtar_format_header * header);
static void mtar_function_list_display2(struct mtar_format_header * header);

void (*mtar_function_list_display)(struct mtar_format_header * header) = mtar_function_list_display1;


void mtar_function_list_configure(const struct mtar_option * option) {
	if (option->verbose > 0)
		mtar_function_list_display = mtar_function_list_display2;
}

void mtar_function_list_display1(struct mtar_format_header * header) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", header->path);
}

void mtar_function_list_display2(struct mtar_format_header * header) {
	char mode[10];
	mtar_file_convert_mode(mode, header->mode);

	static int sug = 0;
	char ug[32];
	strcpy(ug, header->uname);
	strcat(ug, "/");
	strcat(ug, header->gname);

	int tsug = strlen(ug), i;
	if (sug == 0)
		sug = tsug;

	for (i = tsug; i < sug; i++)
		ug[i] = ' ';
	ug[i] = '\0';

	if (sug < tsug)
		sug = tsug;

	struct tm tmval;
	localtime_r(&header->mtime, &tmval);

	char mtime[16];
	strftime(mtime, 16, "%y-%m-%d %R", &tmval);

	if (header->link[0] != '\0' && !(header->mode & S_IFMT)) {
		mode[0] = 'h';
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %s %s %lld %s link to %s\n", mode, ug, mtime, (long long) header->size, header->path, header->link);
	} else if (S_ISLNK(header->mode)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %s %s %lld %s -> %s\n", mode, ug, mtime, (long long) header->size, header->path, header->link);
	} else {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %s %s %lld %s\n", mode, ug, mtime, (long long) header->size, header->path);
	}
}

