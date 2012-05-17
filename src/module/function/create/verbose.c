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
*  Last modified: Thu, 17 May 2012 23:20:45 +0200                           *
\***************************************************************************/

// strcat, strcpy
#include <string.h>
// S_*
#include <sys/stat.h>
// gettimeofday
#include <sys/time.h>
// localtime_r, strftime
#include <time.h>

#include <mtar/file.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "create.h"

static void mtar_function_create_display1(struct mtar_format_header * header, const char * hardlink, int verify);
static void mtar_function_create_display2(struct mtar_format_header * header, const char * hardlink, int verify);
static void mtar_function_create_display3(struct mtar_format_header * header, const char * hardlink, int verify);
static void mtar_function_create_display_label1(const char * label);
static void mtar_function_create_display_label2(const char * label);
static void mtar_function_create_display_label3(const char * label);
static void mtar_function_create_progress1(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);
static void mtar_function_create_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);


void (*mtar_function_create_display)(struct mtar_format_header * header, const char * hardlink, int verify) = mtar_function_create_display1;
void (*mtar_function_create_display_label)(const char * label) = mtar_function_create_display_label1;
void (*mtar_function_create_progress)(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) = mtar_function_create_progress1;


void mtar_function_create_configure(const struct mtar_option * option) {
	switch (option->verbose) {
		case 0:
			mtar_function_create_display = mtar_function_create_display1;
			mtar_function_create_display_label = mtar_function_create_display_label1;
			mtar_function_create_progress = mtar_function_create_progress1;
			break;

		case 1:
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

void mtar_function_create_display1(struct mtar_format_header * header __attribute__((unused)), const char * hardlink __attribute__((unused)), int verify __attribute__((unused))) {}

void mtar_function_create_display2(struct mtar_format_header * header __attribute__((unused)), const char * hardlink __attribute__((unused)), int verify) {
	if (verify)
		mtar_verbose_printf("Verify ");
	mtar_verbose_printf("%s\n", header->path);
}

void mtar_function_create_display3(struct mtar_format_header * header, const char * hardlink, int verify) {
	char mode[11];
	mtar_file_convert_mode(mode, header->mode);

	char user_group[32];
	strcpy(user_group, header->uname);
	strcat(user_group, "/");
	strcat(user_group, header->gname);

	struct tm tmval;
	localtime_r(&header->mtime, &tmval);

	char mtime[24];
	strftime(mtime, 24, "%Y-%m-%d %R", &tmval);

	static int sug = 0;
	static int nsize = 0;
	int ug1, ug2;
	int size1, size2;

	if (verify)
		mtar_verbose_printf("Verify ");

	if (hardlink) {
		mode[0] = 'h';
		mtar_verbose_printf("%s %n%*s%n %n%*lld%n %s %s link to %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path, hardlink);
	} else if (S_ISLNK(header->mode)) {
		mtar_verbose_printf("%s %n%*s%n %n%*lld%n %s %s -> %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path, header->link);
	} else {
		mtar_verbose_printf("%s %n%*s%n %n%*lld%n %s %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, (long long) header->size, &size2, mtime, header->path);
	}

	sug = ug2 - ug1;
	nsize = size2 - size1;
}

void mtar_function_create_display_label1(const char * label __attribute__((unused))) {}

void mtar_function_create_display_label2(const char * label) {
	mtar_verbose_printf("%s\n", label);
}

void mtar_function_create_display_label3(const char * label) {
	char mode[11];
	strcpy(mode, "V---------");

	time_t current = time(0);

	struct tm tmval;
	localtime_r(&current, &tmval);

	char mtime[24];
	strftime(mtime, 24, "%Y-%m-%d %R", &tmval);

	mtar_verbose_printf("%s %s %s\n", mode, mtime, label);
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

