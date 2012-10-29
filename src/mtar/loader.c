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
*  Last modified: Mon, 29 Oct 2012 20:14:56 +0100                           *
\***************************************************************************/

#define _GNU_SOURCE
// dlclose, dlopen
#include <dlfcn.h>
// glob, globfree
#include <glob.h>
// bool
#include <stdbool.h>
// snprintf
#include <stdio.h>
// free
#include <stdlib.h>
// access
#include <unistd.h>

#include <config.h>
#include <mtar/verbose.h>

#include "loader.h"

static int mtar_loader_load_file(const char * filename);

static bool mtar_loader_loaded = false;


int mtar_loader_load(const char * module, const char * name) {
	if (module == NULL || name == NULL)
		return 1;

	char * path;
	asprintf(&path, "lib%s-%s.so", module, name);

	int ret = mtar_loader_load_file(path);

	free(path);

	return ret;
}

void mtar_loader_load_all(const char * module) {
	char * pattern;
	asprintf(&pattern, PLUGINS_PATH "/lib%s-*.so", module);

	glob_t globbuf;
	globbuf.gl_offs = 0;
	int failed = glob(pattern, GLOB_DOOFFS, 0, &globbuf);
	if (failed) {
		globfree(&globbuf);
		free(pattern);
		return;
	}

	unsigned int i;
	for (i = 0; i < globbuf.gl_pathc; i++)
		mtar_loader_load_file(globbuf.gl_pathv[i]);

	globfree(&globbuf);
	free(pattern);
}

int mtar_loader_load_file(const char * filename) {
	mtar_loader_loaded = false;

	void * cookie = dlopen(filename, RTLD_NOW);
	if (cookie == NULL) {
		mtar_verbose_printf("Error while loading file (%s) => %s\n", filename, dlerror());
		return 3;
	} else if (!mtar_loader_loaded) {
		dlclose(cookie);
		return 4;
	}

	return 0;
}

void mtar_loader_register_ok() {
	mtar_loader_loaded = true;
}

