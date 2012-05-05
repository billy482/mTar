/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Sun, 06 Nov 2011 15:38:33 +0100                           *
\****************************************************************************/

#define _GNU_SOURCE
// signal
#include <signal.h>
// va_end, va_start
#include <stdarg.h>
// dprintf, sscanf, vasprintf, vdprintf
#include <stdio.h>
// calloc, free, getenv, realloc
#include <stdlib.h>
// memcpy, memmove, memset, strchr, strcpy, strlen, strstr
#include <string.h>
// ioctl
#include <sys/ioctl.h>
// struct timeval, gettimeofday
#include <sys/time.h>
// struct winsize
#include <termios.h>
// difftime
#include <time.h>

#include <mtar/option.h>
#include <mtar/util.h>
#include <mtar/verbose.h>

static struct mtar_verbose_print_help {
	char * left;
	size_t left_length;
	short has_short_param;

	char * right;
	size_t right_length;
} * lines = 0;
static unsigned int nb_lines = 0;
static unsigned int max_left_width = 0;
static struct timeval mtar_verbose_progress_begin;
static struct timeval mtar_verbose_progress_end;
static int mtar_verbose_terminal_width = 72;

static void mtar_verbose_init(void) __attribute__((constructor));
static size_t mtar_verbose_strlen(const char * str);
static void mtar_verbose_update_size(int signal);


void mtar_verbose_clean() {
	char buffer[mtar_verbose_terminal_width + 1];
	memset(buffer, ' ', mtar_verbose_terminal_width);
	*buffer = buffer[mtar_verbose_terminal_width - 1] = '\r';
	buffer[mtar_verbose_terminal_width] = '\0';

	dprintf(2, buffer);

	*buffer = '\0';
}

void mtar_verbose_init() {
	mtar_verbose_update_size(0);
	signal(SIGWINCH, mtar_verbose_update_size);
}

void mtar_verbose_print_flush(int tab_level, int new_line) {
	unsigned int i;
	unsigned int width = mtar_verbose_terminal_width - max_left_width - 7;
	for (i = 0; i < nb_lines; i++) {
		unsigned int nb_sub_lines = 0;
		char ** sub_lines = mtar_util_string_justified(lines[i].right, width, &nb_sub_lines);

		unsigned int display_width = max_left_width;
		if (!lines[i].has_short_param) {
			dprintf(2, "    ");
			display_width -= 4;
		}

		dprintf(2, "%*s%-*s : %s\n", tab_level, " ", display_width, lines[i].left, *sub_lines);
		free(*sub_lines);

		unsigned int j;
		for (j = 1; j < nb_sub_lines; j++) {
			dprintf(2, "%*s  %*s %s\n", tab_level, " ", max_left_width, " ", sub_lines[j]);
			free(sub_lines[j]);
		}

		free(sub_lines);
		free(lines[i].left);
	}

	free(lines);
	lines = 0;
	nb_lines = 0;
	max_left_width = 0;

	if (new_line)
		dprintf(2, "\n");
}

void mtar_verbose_print_help(const char * format, ...) {
	va_list args;
	char * buffer = 0;
	va_start(args, format);
	vasprintf(&buffer, format, args);
	va_end(args);

	mtar_util_string_trim(buffer, ' ');
	mtar_util_string_delete_double_char(buffer, ' ');

	lines = realloc(lines, (nb_lines + 1) * sizeof(struct mtar_verbose_print_help));
	lines[nb_lines].left = buffer;

	char * ptr = strchr(buffer, ':');
	*ptr = '\0';
	lines[nb_lines].right = ptr + 1;

	mtar_util_string_trim(buffer, ' ');
	mtar_util_string_trim(ptr + 1, ' ');
	mtar_util_string_delete_double_char(buffer, ' ');
	mtar_util_string_delete_double_char(ptr + 1, ' ');

	lines[nb_lines].left_length = strlen(buffer);
	lines[nb_lines].has_short_param = ((buffer[0] == '-' && buffer[1] != '-') || buffer[0] != '-');
	lines[nb_lines].right_length = strlen(ptr + 1);

	if (!lines[nb_lines].has_short_param)
		lines[nb_lines].left_length += 4;

	if (max_left_width < lines[nb_lines].left_length)
		max_left_width = lines[nb_lines].left_length;

	nb_lines++;
}

void mtar_verbose_printf(const char * format, ...) {
	va_list args;
	va_start(args, format);
	vdprintf(2, format, args);
	va_end(args);
}

void mtar_verbose_progress(const char * format, unsigned long long current, unsigned long long upperLimit) {
	mtar_verbose_stop_timer();

	double inter = difftime(mtar_verbose_progress_end.tv_sec, mtar_verbose_progress_begin.tv_sec) + difftime(mtar_verbose_progress_end.tv_usec, mtar_verbose_progress_begin.tv_usec) / 1000000;
	double pct = ((double) current) / upperLimit;
	double total = inter / pct - inter;
	pct *= 100;

	char buffer[mtar_verbose_terminal_width];
	strcpy(buffer, format);

	char * ptr = 0;
	if ((ptr = strstr(buffer, "%E"))) {
		unsigned long long tmpTotal = total;
		unsigned int totalMilli = 1000 * (total - tmpTotal);
		short totalSecond = tmpTotal % 60;
		tmpTotal /= 60;
		short totalMinute = tmpTotal % 60;
		unsigned int totalHour = tmpTotal / 60;

		char tmp[16];
		snprintf(tmp, 16, "%u:%02u:%02u.%03u", totalHour, totalMinute, totalSecond, totalMilli);
		int length = strlen(tmp);

		do {
			if (length != 2)
				memmove(ptr + length, ptr + 2, strlen(ptr + 2) + 1);
			memcpy(ptr, tmp, length);
		} while ((ptr = strstr(ptr + 1, "%E")));
	}

	if ((ptr = strstr(buffer, "%L"))) {
		unsigned long long tmpInter = inter;
		unsigned int interMilli = 1000 * (inter - tmpInter);
		short interSecond = tmpInter % 60;
		tmpInter /= 60;
		short interMinute = tmpInter % 60;
		unsigned int interHour = tmpInter / 60;

		char tmp[16];
		snprintf(tmp, 16, "%u:%02u:%02u.%03u", interHour, interMinute, interSecond, interMilli);
		int length = strlen(tmp);

		do {
			if (length != 2)
				memmove(ptr + length, ptr + 2, strlen(ptr + 2) + 1);
			memcpy(ptr, tmp, length);
		} while ((ptr = strstr(ptr + 1, "%L")));
	}

	if ((ptr = strstr(buffer, "%P"))) {
		char tmp[16];
		snprintf(tmp, 16, "%7.3f%%%%", pct);
		int length = strlen(tmp);

		do {
			memmove(ptr + length, ptr + 2, strlen(ptr + 2) + 1);
			memcpy(ptr, tmp, length);
		} while ((ptr = strstr(ptr + length, "%p")));
	}

	ptr = buffer;
	if ((ptr = strstr(buffer, "%b"))) {
		size_t length = mtar_verbose_terminal_width - strlen(buffer) + 1;
		unsigned int p = length * current / upperLimit;

		memmove(ptr + length, ptr + 2, mtar_verbose_strlen(ptr + 2) + 1);
		memset(ptr, '=', p);
		memset(ptr + p, '.', length - p);
		if (p < length)
			ptr[p] = '>';
	}

	dprintf(2, buffer);
}

void mtar_verbose_restart_timer() {
	gettimeofday(&mtar_verbose_progress_begin, 0);
}

size_t mtar_verbose_strlen(const char * str) {
	if (!str)
		return 0;

	size_t size = 0;
	const char * ptr;
	for (ptr = str; *ptr; ptr++, size++) {
		if (*ptr == '\r' || *ptr == '\n')
			continue;
	}

	return size;
}

void mtar_verbose_stop_timer() {
	gettimeofday(&mtar_verbose_progress_end, 0);
}

void mtar_verbose_update_size(int signal __attribute__((unused))) {
	mtar_verbose_terminal_width = 72;

	static struct winsize size;
	int status = ioctl(2, TIOCGWINSZ, &size);
	if (!status) {
		mtar_verbose_terminal_width = size.ws_col;
		return;
	}

	status = ioctl(0, TIOCGWINSZ, &size);
	if (!status) {
		mtar_verbose_terminal_width = size.ws_col;
		return;
	}

	char * columns = getenv("COLUMNS");
	if (columns)
		sscanf(columns, "%d", &mtar_verbose_terminal_width);
}

