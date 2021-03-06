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
*  Last modified: Sat, 27 Oct 2012 10:32:23 +0200                           *
\***************************************************************************/

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
#include <mtar/util.h>
#include <mtar/verbose.h>

#include "extract.h"

static void mtar_function_extract_clean1(void);
static void mtar_function_extract_clean2(void);
static void mtar_function_extract_display1(struct mtar_format_header * header);
static void mtar_function_extract_display2(struct mtar_format_header * header);
static void mtar_function_extract_display3(struct mtar_format_header * header);
static void mtar_function_extract_progress1(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);
static void mtar_function_extract_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit);

void (*mtar_function_extract_clean)(void) = mtar_function_extract_clean1;
void (*mtar_function_extract_display)(struct mtar_format_header * header) = mtar_function_extract_display1;
void (*mtar_function_extract_progress)(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) = mtar_function_extract_progress1;

static struct timeval begin = { 0, 0 };
static struct timeval middle = { 0, 0 };
static double current_diff = 0;


static void mtar_function_extract_clean1() {}

static void mtar_function_extract_clean2() {
	gettimeofday(&middle, 0);

	double diff = difftime(middle.tv_sec, begin.tv_sec) + difftime(middle.tv_usec, begin.tv_usec) / 1000000;

	if (diff > 4)
		mtar_verbose_clean();
}

void mtar_function_extract_configure(const struct mtar_option * option) {
	switch (option->verbose) {
		case 0:
			mtar_function_extract_clean = mtar_function_extract_clean1;
			mtar_function_extract_display = mtar_function_extract_display1;
			mtar_function_extract_progress = mtar_function_extract_progress1;
			break;

		case 1:
			mtar_function_extract_clean = mtar_function_extract_clean1;
			mtar_function_extract_display = mtar_function_extract_display2;
			mtar_function_extract_progress = mtar_function_extract_progress1;
			break;

		default:
			mtar_function_extract_clean = mtar_function_extract_clean2;
			mtar_function_extract_display = mtar_function_extract_display3;
			mtar_function_extract_progress = mtar_function_extract_progress2;
			break;
	}
}

static void mtar_function_extract_display1(struct mtar_format_header * header __attribute__((unused))) {}

static void mtar_function_extract_display2(struct mtar_format_header * header) {
	mtar_verbose_printf("%s\n", header->path);
}

static void mtar_function_extract_display3(struct mtar_format_header * header) {
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
		mtar_verbose_printf("%s %s --Volume Header: %s--\n", mode, mtime, header->path);
	} else if (S_ISREG(header->mode) && header->path && header->link) {
		mode[0] = 'h';
		mtar_verbose_printf("%s %n%*s%n %n%*zd%n %s %s link to %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, header->size, &size2, mtime, header->path, header->link);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	} else if (S_ISLNK(header->mode)) {
		mtar_verbose_printf("%s %n%*s%n %n%*zd%n %s %s -> %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, header->size, &size2, mtime, header->path, header->link);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	} else {
		mtar_verbose_printf("%s %n%*s%n %n%*zd%n %s %s\n", mode, &ug1, sug, user_group, &ug2, &size1, nsize, header->size, &size2, mtime, header->path);

		sug = ug2 - ug1;
		nsize = size2 - size1;
	}
}

static void mtar_function_extract_progress1(const char * filename __attribute__((unused)), const char * format __attribute__((unused)), unsigned long long current __attribute__((unused)), unsigned long long upperLimit __attribute__((unused))) {}

static void mtar_function_extract_progress2(const char * filename, const char * format, unsigned long long current, unsigned long long upperLimit) {
	static unsigned long long current_hash = 0;
	unsigned long long hash = mtar_util_compute_hash_string(filename);

	if (current_hash != hash) {
		current_hash = hash;
		gettimeofday(&begin, 0);
		current_diff = 4;
		mtar_verbose_restart_timer();
		return;
	}

	gettimeofday(&middle, 0);

	double diff = difftime(middle.tv_sec, begin.tv_sec) + difftime(middle.tv_usec, begin.tv_usec) / 1000000;

	if (diff > current_diff) {
		current_diff = diff + 1;
		mtar_verbose_progress(format, current, upperLimit);
	}
}

