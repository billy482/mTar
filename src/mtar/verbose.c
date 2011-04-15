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
*  Last modified: Fri, 15 Apr 2011 23:13:00 +0200                       *
\***********************************************************************/

// dprintf, vdprintf
#include <stdio.h>
// signal
#include <signal.h>
// memset
#include <string.h>
// ioctl
#include <sys/ioctl.h>
// struct winsize
#include <termios.h>
// va_end, va_start
#include <stdarg.h>

#include "option.h"
#include "verbose.h"

static void verbose_clean(void);
static void verbose_init(void);
static void verbose_noClean(void);
static void verbose_noPrintf(const char * format, ...);
static void verbose_updateSize(int signal);

static int verbose_terminalWidth = 72;


void mtar_verbose_get(struct mtar_verbose * verbose, struct mtar_option * option) {
	switch (option->verbose) {
		default:
		case 0:
			verbose->clean = verbose_noClean;
			verbose->print = verbose_noPrintf;
			break;

		case 1:
			verbose->clean = verbose_clean;
			verbose->print = verbose_noPrintf;
			break;

		case 2:
			verbose->clean = verbose_clean;
			verbose->print = verbose_noPrintf;
			break;
	}
}

void mtar_verbose_printf(const char * format, ...) {
	va_list args;
	va_start(args, format);
	vdprintf(1, format, args);
	va_end(args);
}

void verbose_clean() {
	char buffer[verbose_terminalWidth];
	memset(buffer, ' ', verbose_terminalWidth);
	*buffer = buffer[verbose_terminalWidth - 1] = '\r';

	dprintf(1, buffer);
}

__attribute__((constructor))
void verbose_init() {
	verbose_updateSize(0);
	signal(SIGWINCH, verbose_updateSize);
}

void verbose_noClean() {}

void verbose_noPrintf(const char * format __attribute__((unused)), ...) {}

void verbose_updateSize(int signal __attribute__((unused))) {
	static struct winsize size;
	int status = ioctl(1, TIOCGWINSZ, &size);
	if (!status)
		verbose_terminalWidth = size.ws_col;
}

