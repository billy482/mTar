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
*  Last modified: Wed, 11 May 2011 14:09:37 +0200                       *
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

static void mtar_verbose_init(void);
static size_t mtar_verbose_strlen(const char * str);
static void mtar_verbose_updateSize(int signal);

static enum mtar_verbose_level mtar_verbose_level = MTAR_VERBOSE_LEVEL_ERROR;
static int mtar_verbose_terminalWidth = 72;

static struct timeval mtar_verbose_progress_begin;
static struct timeval mtar_verbose_progress_end;


void mtar_verbose_clean() {
	char buffer[mtar_verbose_terminalWidth + 1];
	memset(buffer, ' ', mtar_verbose_terminalWidth);
	*buffer = buffer[mtar_verbose_terminalWidth - 1] = '\r';
	buffer[mtar_verbose_terminalWidth] = '\0';

	dprintf(2, buffer);

	*buffer = '\0';
}

void mtar_verbose_configure(const struct mtar_option * option) {
	mtar_verbose_level = option->verbose;
}

__attribute__((constructor))
void mtar_verbose_init() {
	mtar_verbose_updateSize(0);
	signal(SIGWINCH, mtar_verbose_updateSize);
}

void mtar_verbose_printf(enum mtar_verbose_level level, const char * format, ...) {
	if (mtar_verbose_level < level)
		return;

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

	char buffer[mtar_verbose_terminalWidth];
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
		size_t length = mtar_verbose_terminalWidth - strlen(buffer) + 1;
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

void mtar_verbose_updateSize(int signal __attribute__((unused))) {
	static struct winsize size;
	int status = ioctl(2, TIOCGWINSZ, &size);
	if (!status)
		mtar_verbose_terminalWidth = size.ws_col;
	else
		mtar_verbose_terminalWidth = 72;
}

