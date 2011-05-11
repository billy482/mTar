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
*  Last modified: Wed, 11 May 2011 14:00:54 +0200                       *
\***********************************************************************/

// dlclose, dlerror, dlopen
#include <dlfcn.h>
// snprintf
#include <stdio.h>
// access
#include <unistd.h>

#include <config.h>

#include "loader.h"

static short mtar_loader_loaded = 0;


int mtar_loader_load(const char * module, const char * name) {
	if (!module || !name)
		return 1;

	char path[256];
	snprintf(path, 256, "%s/lib%s-%s.so", PLUGINS_PATH, module, name);

	if (access(path, R_OK | X_OK)) {
		return 2;
	}

	mtar_loader_loaded = 0;

	void * cookie = dlopen(path, RTLD_NOW);
	if (!cookie) {
		return 3;
	} else if (!mtar_loader_loaded) {
		return 4;
	}

	return 0;
}

void mtar_loader_register_ok() {
	mtar_loader_loaded = 1;
}

