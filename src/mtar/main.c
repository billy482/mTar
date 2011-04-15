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
*  Last modified: Fri, 15 Apr 2011 12:40:22 +0200                       *
\***********************************************************************/

// printf
#include <stdio.h>
// strlen, strrchr, strspn
#include <string.h>

#include "option.h"

static void showHelp(const char * path);
static void showVersion(const char * path);

int main(int argc, char ** argv) {
	if (argc < 2) {
		showHelp(*argv);
		return 1;
	}

	size_t length = strlen(argv[1]);
	size_t goodArg = strspn(argv[1], "-fhV");
	if (length != goodArg) {
		printf("Invalid argument '%c'\n", argv[1][goodArg]);
		showHelp(*argv);
		return 1;
	}

	static struct mtar_option option;
	init_option(&option);

	unsigned int i;
	int optArg = 2;
	for (i = 3; i < length; i++) {
		switch (argv[1][i]) {
			case 'f':
				if (optArg >= argc) {
					printf("Argument 'f' require a parameter\n");
					showHelp(*argv);
					return 1;
				}
				option.filename = argv[optArg++];
				break;

			case 'h':
				showHelp(*argv);
				return 0;

			case 'V':
				showVersion(*argv);
				return 0;
		}
	}

	return 0;
}

void showHelp(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	printf("%s: modular tar (version: %s)\n", ptr, MTAR_VERSION);
}

void showVersion(const char * path) {
	const char * ptr = strrchr(path, '/');
	if (ptr)
		ptr++;
	else
		ptr = path;

	printf("%s: modular tar (version: %s, build: %s %s)\n", ptr, MTAR_VERSION, __DATE__, __TIME__);
}

