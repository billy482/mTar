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
*  Last modified: Tue, 26 Jul 2011 16:34:17 +0200                       *
\***********************************************************************/

// gettimeofday
#include <sys/time.h>
// strcat, strcpy
#include <string.h>
// S_*
#include <sys/stat.h>
// localtime_r, strftime
#include <time.h>

#include <mtar/file.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "common.h"

static void mtar_function_extract_display1(struct mtar_format_header * header);
static void mtar_function_extract_display2(struct mtar_format_header * header);
static void mtar_function_extract_display3(struct mtar_format_header * header);
static void mtar_function_extract_progress1(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);
static void mtar_function_extract_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);

void (*mtar_function_extract_display)(struct mtar_format_header * header) = mtar_function_extract_display1;
void (*mtar_function_extract_progress)(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) = mtar_function_extract_progress1;


void mtar_function_extract_configure(const struct mtar_option * option) {
	switch (option->verbose) {
		case MTAR_VERBOSE_LEVEL_ERROR:
			mtar_function_extract_display = mtar_function_extract_display1;
			mtar_function_extract_progress = mtar_function_extract_progress1;
			break;

		case MTAR_VERBOSE_LEVEL_WARNING:
			mtar_function_extract_display = mtar_function_extract_display2;
			mtar_function_extract_progress = mtar_function_extract_progress1;
			break;

		default:
			mtar_function_extract_display = mtar_function_extract_display3;
			mtar_function_extract_progress = mtar_function_extract_progress2;
			break;
	}
}

void mtar_function_extract_display1(struct mtar_format_header * header __attribute__((unused))) {}

void mtar_function_extract_display2(struct mtar_format_header * header) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", header->path);
}

void mtar_function_extract_display3(struct mtar_format_header * header) {
	char mode[11];
	mtar_file_convert_mode(mode, header->mode);

	char user_group[32];
	if (!header->is_label) {
		strcpy(user_group, header->uname);
		strcat(user_group, "/");
		strcat(user_group, header->gname);
	}

	struct tm tmval;
	localtime_r(&header->mtime, &tmval);

	char mtime[24];
	strftime(mtime, 24, "%Y-%m-%d %R", &tmval);

	static int sug = 0;
	static int nsize = 0;
	int ug1, ug2;
	int size1, size2;

	if (header->is_label) {
		mode[0] = 'V';
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %s --Volume Header: %s--\n", mode, mtime, header->path);
	} else if (header->link[0] != '\0' && !(header->mode & S_IFMT)) {
		mode[0] = 'h';
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s link to %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path, header->link);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	} else if (S_ISLNK(header->mode)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s -> %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path, header->link);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	} else {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	}
}

void mtar_function_extract_progress1(const char * filename __attribute__((unused)), const char * format __attribute__((unused)), unsigned long long current __attribute__((unused)), unsigned long long upperLimit __attribute__((unused))) {}

void mtar_function_extract_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) {
	static const char * current_file = 0;
	static struct timeval last = {0, 0};

	struct timeval curtime;
	gettimeofday(&curtime, 0);

	if (current_file == 0) {
		current_file = filename;
		last = curtime;
		mtar_verbose_restart_timer();
		return;
	}

	double diff = difftime(curtime.tv_sec, last.tv_sec) + difftime(curtime.tv_usec, last.tv_usec) / 1000000;

	if (filename != current_file) {
		if (diff < 2)
			return;

		current_file = filename;
		last = curtime;
		mtar_verbose_restart_timer();
	} else {
		if (diff < 1)
			return;

		last = curtime;
	}

	mtar_verbose_progress(format, current, upperLimit);
}

