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
*  Last modified: Fri, 19 Oct 2012 23:50:58 +0200                           *
\***************************************************************************/

// free, realloc
#include <stdlib.h>
// bzero, strcmp, strlen
#include <string.h>

#include <mtar/filter.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "format.h"
#include "loader.h"

static void mtar_format_exit(void) __attribute__((destructor));
static struct mtar_format * mtar_format_get(const char * name);

static struct mtar_format ** mtar_format_formats = NULL;
static unsigned int mtar_format_nb_formats = 0;


void mtar_format_exit() {
	if (mtar_format_nb_formats > 0)
		free(mtar_format_formats);
	mtar_format_formats = NULL;
}

struct mtar_format * mtar_format_get(const char * name) {
	unsigned int i;
	for (i = 0; i < mtar_format_nb_formats; i++)
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];

	if (mtar_loader_load("format", name))
		return NULL;

	for (i = 0; i < mtar_format_nb_formats; i++)
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];

	return NULL;
}

struct mtar_format_in * mtar_format_get_in(const struct mtar_option * option) {
	if (option == NULL)
		return NULL;

	struct mtar_io_in * io = mtar_filter_get_in(option);
	if (io != NULL)
		return mtar_format_get_in2(io, option);

	return NULL;
}

struct mtar_format_in * mtar_format_get_in2(struct mtar_io_in * io, const struct mtar_option * option) {
	if (option == NULL || io == NULL)
		return NULL;

	struct mtar_format * format = mtar_format_get(option->format);
	if (format != NULL)
		return format->new_in(io, option);

	return NULL;
}

struct mtar_format_out * mtar_format_get_out(const struct mtar_option * option) {
	if (option == NULL)
		return NULL;

	struct mtar_io_out * io = mtar_filter_get_out(option);
	if (io != NULL)
		return mtar_format_get_out2(io, option);

	return NULL;
}

struct mtar_format_out * mtar_format_get_out2(struct mtar_io_out * io, const struct mtar_option * option) {
	if (option == NULL || io == NULL)
		return NULL;

	struct mtar_format * format = mtar_format_get(option->format);
	if (format != NULL)
		return format->new_out(io, option);

	return NULL;
}

void mtar_format_free_header(struct mtar_format_header * h) {
	if (h == NULL)
		return;

	free(h->path);
	h->path = NULL;

	free(h->link);
	h->link = NULL;
}

void mtar_format_init_header(struct mtar_format_header * h) {
	if (h == NULL)
		return;

	h->dev = 0;
	h->path = 0;
	h->link = 0;
	h->size = 0;
	h->position = 0;
	h->mode = 0;
	h->mtime = 0;
	h->uid = 0;
	bzero(h->uname, 32);
	h->gid = 0;
	bzero(h->gname, 32);
	h->is_label = 0;
}

void mtar_format_register(struct mtar_format * f) {
	if (f == NULL || mtar_plugin_check(&f->api_level))
		return;

	/**
	 * check if module has been preciously loaded
	 * or another module has the same name
	 */
	unsigned int i;
	for (i = 0; i < mtar_format_nb_formats; i++)
		if (mtar_format_formats[i] == f || !strcmp(f->name, mtar_format_formats[i]->name))
			return;

	mtar_format_formats = realloc(mtar_format_formats, (mtar_format_nb_formats + 1) * (sizeof(struct mtar_format *)));
	mtar_format_formats[mtar_format_nb_formats] = f;
	mtar_format_nb_formats++;

	mtar_loader_register_ok();
}

void mtar_format_show_description() {
	mtar_loader_load_all("format");

	unsigned int i;
	for (i = 0; i < mtar_format_nb_formats; i++)
		mtar_format_formats[i]->show_description();
}

void mtar_format_show_version() {
	mtar_loader_load_all("format");
	mtar_verbose_printf("List of available backend formats :\n");

	unsigned int i;
	for (i = 0; i < mtar_format_nb_formats; i++)
		mtar_format_formats[i]->show_version();

	mtar_verbose_printf("\n");
}

