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
*  Last modified: Mon, 18 Apr 2011 08:59:58 +0200                       *
\***********************************************************************/

// realloc
#include <stdlib.h>
// strcmp
#include <string.h>

#include "function.h"
#include "loader.h"

static struct function {
	const char * name;
	mtar_function function;
} * functions = 0;
static unsigned int nbFunctions = 0;


mtar_function mtar_function_get(const char * name) {
	unsigned int i;
	for (i = 0; i < nbFunctions; i++) {
		if (!strcmp(name, functions[i].name))
			return functions[i].function;
	}
	if (loader_load("function", name))
		return 0;
	for (i = 0; i < nbFunctions; i++) {
		if (!strcmp(name, functions[i].name))
			return functions[i].function;
	}
	return 0;
}

void mtar_function_register(const char * name, mtar_function f) {
	functions = realloc(functions, (nbFunctions + 1) * sizeof(struct function));
	functions[nbFunctions].name = name;
	functions[nbFunctions].function = f;
	nbFunctions++;

	loader_register_ok();
}

