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
*  Last modified: Tue, 03 May 2011 13:36:53 +0200                       *
\***********************************************************************/

// snprintf
#include <stdio.h>
// free, malloc
#include <stdlib.h>
// strlen, strncpy
#include <string.h>
// stat
#include <sys/stat.h>
// stat
#include <sys/types.h>
// localtime_r
#include <time.h>
// stat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "verbose.h"

static void mtar_function_create_display1(const char * filename, struct stat * st);
static void mtar_function_create_display2(const char * filename, struct stat * st);
static void mtar_function_create_display3(const char * filename, struct stat * st);
static void mtar_function_create_progress1(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);
static void mtar_function_create_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);


void (*mtar_function_create_display)(const char * filename, struct stat * st) = mtar_function_create_display1;
void (*mtar_function_create_progress)(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) = mtar_function_create_progress1;


void mtar_function_create_configure(const struct mtar_option * option) {
	switch (option->verbose) {
		case MTAR_VERBOSE_LEVEL_ERROR:
			mtar_function_create_display = mtar_function_create_display1;
			mtar_function_create_progress = mtar_function_create_progress1;
			break;

		case MTAR_VERBOSE_LEVEL_WARNING:
			mtar_function_create_display = mtar_function_create_display2;
			mtar_function_create_progress = mtar_function_create_progress1;
			break;

		default:
			mtar_function_create_display = mtar_function_create_display3;
			mtar_function_create_progress = mtar_function_create_progress2;
			break;
	}
}

void mtar_function_create_display1(const char * filename __attribute__((unused)), struct stat * st __attribute__((unused))) {}

void mtar_function_create_display2(const char * filename, struct stat * st __attribute__((unused))) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", filename);
}

void mtar_function_create_display3(const char * filename, struct stat * st) {
	char * buffer = malloc(128);
	char * pos = buffer + 10;
	struct tm tmval;

	mtar_file_convert_mode(buffer, st->st_mode);

	*pos = ' ';
	pos++;

	mtar_file_uid2name(pos, 128 - (pos - buffer), st->st_uid);
	pos += strlen(pos);

	*pos = '/';
	pos++;

	mtar_file_gid2name(pos, 128 - (pos - buffer), st->st_gid);
	pos += strlen(pos);

	snprintf(pos, 128 - (pos - buffer), " %lld ", (long long) st->st_size);
	pos += strlen(pos);

	localtime_r(&st->st_mtime, &tmval);
	strftime(pos, 128 - (pos - buffer), "%y-%m-%d %R ", &tmval);
	pos += strlen(pos);

	strncpy(pos, filename, 128 - (pos - buffer));

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", buffer);
	free(buffer);
}

void mtar_function_create_progress1(const char * filename __attribute__((unused)), const char * format __attribute__((unused)), unsigned long long current __attribute__((unused)), unsigned long long upperLimit __attribute__((unused))) {}

void mtar_function_create_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) {
	static const char * current_file = 0;
	static time_t start = 0, last = 0;

	time_t curtime = time(0);

	if (start + 2 > curtime)
		return;

	if (last >= curtime)
		return;

	last = curtime;

	if (filename != current_file) {
		current_file = filename;
		//last_start = time(0);
		mtar_verbose_restart_timer();
		return;
	}

	mtar_verbose_progress(format, current, upperLimit);
}
