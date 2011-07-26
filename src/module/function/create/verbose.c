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
*  Last modified: Tue, 26 Jul 2011 15:40:24 +0200                       *
\***********************************************************************/

// snprintf
#include <stdio.h>
// free, malloc
#include <stdlib.h>
// strlen, strncpy
#include <string.h>
// stat
#include <sys/stat.h>
// gettimeofday
#include <sys/time.h>
// stat
#include <sys/types.h>
// localtime_r, strftime
#include <time.h>
// readlink, stat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "common.h"

static void mtar_function_create_display1(const char * filename, struct stat * st, const char * hardlink);
static void mtar_function_create_display2(const char * filename, struct stat * st, const char * hardlink);
static void mtar_function_create_display3(const char * filename, struct stat * st, const char * hardlink);
static void mtar_function_create_display_label1(const char * label);
static void mtar_function_create_display_label2(const char * label);
static void mtar_function_create_display_label3(const char * label);
static void mtar_function_create_progress1(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);
static void mtar_function_create_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);


void (*mtar_function_create_display)(const char * filename, struct stat * st, const char * hardlink) = mtar_function_create_display1;
void (*mtar_function_create_display_label)(const char * label) = mtar_function_create_display_label1;
void (*mtar_function_create_progress)(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) = mtar_function_create_progress1;


void mtar_function_create_configure(const struct mtar_option * option) {
	switch (option->verbose) {
		case MTAR_VERBOSE_LEVEL_ERROR:
			mtar_function_create_display = mtar_function_create_display1;
			mtar_function_create_display_label = mtar_function_create_display_label1;
			mtar_function_create_progress = mtar_function_create_progress1;
			break;

		case MTAR_VERBOSE_LEVEL_WARNING:
			mtar_function_create_display = mtar_function_create_display2;
			mtar_function_create_display_label = mtar_function_create_display_label2;
			mtar_function_create_progress = mtar_function_create_progress1;
			break;

		default:
			mtar_function_create_display = mtar_function_create_display3;
			mtar_function_create_display_label = mtar_function_create_display_label3;
			mtar_function_create_progress = mtar_function_create_progress2;
			break;
	}
}

void mtar_function_create_display1(const char * filename __attribute__((unused)), struct stat * st __attribute__((unused)), const char * hardlink __attribute__((unused))) {}

void mtar_function_create_display2(const char * filename, struct stat * st __attribute__((unused)), const char * hardlink __attribute__((unused))) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", filename);
}

void mtar_function_create_display3(const char * filename, struct stat * st, const char * hardlink) {
	char mode[11];
	mtar_file_convert_mode(mode, st->st_mode);

	char user_group[32];
	mtar_file_uid2name(user_group, 32, st->st_uid);
	strcat(user_group, "/");
	size_t length = strlen(user_group);
	mtar_file_gid2name(user_group + length, 32 - length, st->st_gid);

	struct tm tmval;
	localtime_r(&st->st_mtime, &tmval);

	char mtime[24];
	strftime(mtime, 24, "%Y-%m-%d %R", &tmval);

	static int sug = 0;
	static int nsize = 0;
	int ug1, ug2;
	int size1, size2;

	if (hardlink) {
		mode[0] = 'h';
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s link to %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) st->st_size, &size2, mtime, filename, hardlink);
	} else if (S_ISLNK(st->st_mode)) {
		char link[256];
		ssize_t size_link = readlink(filename, link, 256);
		link[size_link] = '\0';

		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s -> %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) st->st_size, &size2, mtime, filename, link);
	} else {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %n%*s%n %n%*lld%n %s %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) st->st_size, &size2, mtime, filename);
	}

	sug = ug2 - ug1;
	nsize = size2 - size1;
}

void mtar_function_create_display_label1(const char * label __attribute__((unused))) {}

void mtar_function_create_display_label2(const char * label) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s\n", label);
}

void mtar_function_create_display_label3(const char * label) {
	char mode[11];
	strcpy(mode, "V---------");

	time_t current = time(0);

	struct tm tmval;
	localtime_r(&current, &tmval);

	char mtime[24];
	strftime(mtime, 24, "%Y-%m-%d %R", &tmval);

	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s %s %s\n", mode, mtime, label);
}

void mtar_function_create_progress1(const char * filename __attribute__((unused)), const char * format __attribute__((unused)), unsigned long long current __attribute__((unused)), unsigned long long upperLimit __attribute__((unused))) {}

void mtar_function_create_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) {
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

