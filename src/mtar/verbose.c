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
*  Last modified: Thu, 21 Apr 2011 16:53:27 +0200                       *
\***********************************************************************/

// va_end, va_start
#include <stdarg.h>
// dprintf, vdprintf
#include <stdio.h>
// signal
#include <signal.h>
// memset, strcpy
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

#include "verbose.h"

static void verbose_init(void);
static void verbose_updateSize(int signal);

static enum mtar_verbose_level verbose_level = MTAR_VERBOSE_LEVEL_ERROR;
static int verbose_terminalWidth = 72;

static struct timeval verbose_progress_begin;
static struct timeval verbose_progress_end;


void mtar_verbose_clean() {
	char buffer[verbose_terminalWidth];
	memset(buffer, ' ', verbose_terminalWidth);
	*buffer = buffer[verbose_terminalWidth - 1] = '\r';

	dprintf(2, buffer);
}

void mtar_verbose_configure(struct mtar_option * option) {
	verbose_level = option->verbose;
}

void mtar_verbose_printf(enum mtar_verbose_level level, const char * format, ...) {
	if (verbose_level < level)
		return;

	va_list args;
	va_start(args, format);
	vdprintf(2, format, args);
	va_end(args);
}

void mtar_verbose_progress(const char * format, unsigned long long current, unsigned long long upperLimit) {
	mtar_verbose_stop_timer();

	double inter = difftime(verbose_progress_end.tv_sec, verbose_progress_begin.tv_sec) + difftime(verbose_progress_end.tv_usec, verbose_progress_begin.tv_usec) / 1000000;
	double pct = ((double) current) / upperLimit;
	double total = inter / pct - inter;
	pct *= 100;

	char buffer[verbose_terminalWidth];
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

	if ((ptr = strstr(buffer, "%p"))) {
		char tmp[16];
		snprintf(tmp, 16, "% 6.3f%%%%", pct);
		int length = strlen(tmp);

		do {
			if (length != 2)
				memmove(ptr + length, ptr + 2, strlen(ptr + 2) + 1);
			memcpy(ptr, tmp, length);
		} while ((ptr = strstr(ptr + length, "%p")));
	}

	ptr = buffer;
	while ((ptr = strchr(ptr, '%'))) {
		int s = 0, t = 0;
		if (sscanf(ptr + 1, "%db%n", &s, &t) > 0) {
			t++;
			int p = s * current / upperLimit;

			if (s != t)
				memmove(ptr + s, ptr + t, strlen(ptr + t) + 1);
			memset(ptr, '#', p);
			memset(ptr + p, '.', s - p);
		}
		ptr++;
	}

	dprintf(2, buffer);
}

void mtar_verbose_restart_timer() {
	gettimeofday(&verbose_progress_begin, 0);
}

void mtar_verbose_stop_timer() {
	gettimeofday(&verbose_progress_end, 0);
}


__attribute__((constructor))
void verbose_init() {
	verbose_updateSize(0);
	signal(SIGWINCH, verbose_updateSize);
}

void verbose_updateSize(int signal __attribute__((unused))) {
	static struct winsize size;
	int status = ioctl(2, TIOCGWINSZ, &size);
	if (!status)
		verbose_terminalWidth = size.ws_col;
	else
		verbose_terminalWidth = 72;
}

