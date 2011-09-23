/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Fri, 23 Sep 2011 17:21:42 +0200                           *
\***************************************************************************/

// free, realloc
#include <stdlib.h>
// bzero, strcmp, strlen
#include <string.h>

#include <mtar/verbose.h>

#include "filter.h"
#include "format.h"
#include "io.h"
#include "loader.h"
#include "option.h"

static void mtar_format_exit(void) __attribute__((destructor));
static struct mtar_format * mtar_format_get(const char * name);

static struct mtar_format ** mtar_format_formats = 0;
static unsigned int mtar_format_nb_formats = 0;


void mtar_format_exit() {
	if (mtar_format_nb_formats > 0)
		free(mtar_format_formats);
	mtar_format_formats = 0;
}

struct mtar_format * mtar_format_get(const char * name) {
	unsigned int i;
	for (i = 0; i < mtar_format_nb_formats; i++)
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];
	if (mtar_loader_load("format", name))
		return 0;
	for (i = 0; i < mtar_format_nb_formats; i++)
		if (!strcmp(name, mtar_format_formats[i]->name))
			return mtar_format_formats[i];
	return 0;
}

struct mtar_format_in * mtar_format_get_in(const struct mtar_option * option) {
	if (!option)
		return 0;

	struct mtar_io_in * io = mtar_filter_get_in(option);
	if (io)
		return mtar_format_get_in2(io, option);
	return 0;
}

struct mtar_format_in * mtar_format_get_in2(struct mtar_io_in * io, const struct mtar_option * option) {
	if (!option || !io)
		return 0;

	struct mtar_format * format = mtar_format_get(option->format);
	if (format)
		return format->new_in(io, option);
	return 0;
}

struct mtar_format_out * mtar_format_get_out(const struct mtar_option * option) {
	if (!option)
		return 0;

	struct mtar_io_out * io = mtar_filter_get_out(option);
	if (io)
		return mtar_format_get_out2(io, option);
	return 0;
}

struct mtar_format_out * mtar_format_get_out2(struct mtar_io_out * io, const struct mtar_option * option) {
	if (!option || !io)
		return 0;

	struct mtar_format * format = mtar_format_get(option->format);
	if (format)
		return format->new_out(io, option);
	return 0;
}

void mtar_format_init_header(struct mtar_format_header * h) {
	if (!h)
		return;

	h->dev = 0;
	bzero(h->path, 256);
	h->filename = 0;
	bzero(h->link, 256);
	h->size = 0;
	h->mode = 0;
	h->mtime = 0;
	h->uid = 0;
	bzero(h->uname, 32);
	h->gid = 0;
	bzero(h->gname, 32);
	h->is_label = 0;
}

void mtar_format_register(struct mtar_format * f) {
	if (!f || f->api_version != MTAR_FORMAT_API_VERSION)
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
	mtar_loader_loadAll("format");

	unsigned int i, length = 0;
	for (i = 0; i < mtar_format_nb_formats; i++) {
		unsigned int ll = strlen(mtar_format_formats[i]->name);
		if (ll > length)
			length = ll;
	}

	for (i = 0; i < mtar_format_nb_formats; i++) {
		mtar_verbose_printf("    %-*s : ", length, mtar_format_formats[i]->name);
		mtar_format_formats[i]->show_description();
	}
}

