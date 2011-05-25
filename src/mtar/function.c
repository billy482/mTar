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
*  Last modified: Tue, 24 May 2011 17:12:18 +0200                       *
\***********************************************************************/

// free, realloc
#include <stdlib.h>
// strcmp
#include <string.h>

#include "function.h"
#include "loader.h"
#include "verbose.h"

static void mtar_function_exit(void);

static struct function {
	const char * name;
	struct mtar_function * function;
} * mtar_function_functions = 0;
static unsigned int mtar_function_nbFunctions = 0;


__attribute__((destructor))
void mtar_function_exit() {
	if (mtar_function_nbFunctions > 0)
		free(mtar_function_functions);
	mtar_function_functions = 0;
}

mtar_function_f mtar_function_get(const char * name) {
	unsigned int i;
	for (i = 0; i < mtar_function_nbFunctions; i++) {
		if (!strcmp(name, mtar_function_functions[i].name))
			return mtar_function_functions[i].function->doWork;
	}
	if (mtar_loader_load("function", name))
		return 0;
	for (i = 0; i < mtar_function_nbFunctions; i++) {
		if (!strcmp(name, mtar_function_functions[i].name))
			return mtar_function_functions[i].function->doWork;
	}
	return 0;
}

void mtar_function_register(const char * name, struct mtar_function * f) {
	mtar_function_functions = realloc(mtar_function_functions, (mtar_function_nbFunctions + 1) * sizeof(struct function));
	mtar_function_functions[mtar_function_nbFunctions].name = name;
	mtar_function_functions[mtar_function_nbFunctions].function = f;
	mtar_function_nbFunctions++;

	mtar_loader_register_ok();
}

void mtar_function_showDescription() {
	mtar_loader_loadAll("function");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\nList of available functions :\n");

	unsigned int i;
	for (i = 0; i < mtar_function_nbFunctions; i++)
		mtar_function_functions[i].function->showDescription();
}

