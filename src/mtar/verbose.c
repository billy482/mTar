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
*  Last modified: Wed, 20 Apr 2011 22:56:40 +0200                       *
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

#include <mtar/option.h>

#include "verbose.h"

static void verbose_init(void);
static void verbose_updateSize(int signal);

static enum mtar_verbose_level verbose_level = MTAR_VERBOSE_LEVEL_ERROR;
static int verbose_terminalWidth = 72;


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
}





static void verbose_noClean(void);
static void verbose_noPrintf(const char * format, ...);

void mtar_verbose_get(struct mtar_verbose * verbose, struct mtar_option * option) {}

void verbose_noPrintf(const char * format __attribute__((unused)), ...) {}

