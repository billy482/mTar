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
*  Last modified: Fri, 19 Oct 2012 23:55:49 +0200                           *
\***************************************************************************/

// free, realloc
#include <stdlib.h>
// strcmp
#include <string.h>

#include <mtar/verbose.h>

#include "function.h"
#include "loader.h"

static void mtar_function_exit(void) __attribute__((destructor));

struct mtar_function ** mtar_function_functions = NULL;
static unsigned int mtar_function_nb_functions = 0;


void mtar_function_exit() {
	if (mtar_function_nb_functions > 0)
		free(mtar_function_functions);
	mtar_function_functions = NULL;
}

mtar_function_f mtar_function_get(const char * name) {
	unsigned int i;
	for (i = 0; i < mtar_function_nb_functions; i++)
		if (!strcmp(name, mtar_function_functions[i]->name))
			return mtar_function_functions[i]->do_work;

	if (mtar_loader_load("function", name))
		return NULL;

	for (i = 0; i < mtar_function_nb_functions; i++)
		if (!strcmp(name, mtar_function_functions[i]->name))
			return mtar_function_functions[i]->do_work;

	return NULL;
}

void mtar_function_register(struct mtar_function * f) {
	if (f == NULL || mtar_plugin_check(&f->api_level))
		return;

	unsigned int i;
	for (i = 0; i < mtar_function_nb_functions; i++)
		if (!strcmp(f->name, mtar_function_functions[i]->name))
			return;

	mtar_function_functions = realloc(mtar_function_functions, (mtar_function_nb_functions + 1) * sizeof(struct mtar_function *));
	mtar_function_functions[mtar_function_nb_functions] = f;
	mtar_function_nb_functions++;

	mtar_loader_register_ok();
}

void mtar_function_show_description() {
	mtar_loader_load_all("function");

	unsigned int i;
	for (i = 0; i < mtar_function_nb_functions; i++)
		mtar_function_functions[i]->show_description();
}

void mtar_function_show_help(const char * function) {
	unsigned int i;
	struct mtar_function * f = 0;
	for (i = 0; f == NULL && i < mtar_function_nb_functions; i++)
		if (!strcmp(mtar_function_functions[i]->name, function))
			f = mtar_function_functions[i];

	if (f == NULL) {
		mtar_loader_load("function", function);

		for (i = 0; !f && i < mtar_function_nb_functions; i++)
			if (!strcmp(mtar_function_functions[i]->name, function))
				f = mtar_function_functions[i];
	}

	if (f != NULL) {
		mtar_verbose_printf("Help for function: %s\n", function);
		f->show_help();
	} else {
		mtar_verbose_printf("No help found for function: %s\n", function);
	}
}

void mtar_function_show_version() {
	mtar_loader_load_all("function");
	mtar_verbose_printf("List of available backend functions :\n");

	unsigned int i;
	for (i = 0; i < mtar_function_nb_functions; i++)
		mtar_function_functions[i]->show_version();

	mtar_verbose_printf("\n");
}

