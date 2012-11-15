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
*  Last modified: Thu, 15 Nov 2012 19:26:16 +0100                           *
\***************************************************************************/

// O_RDONLY, O_RDWR, O_TRUNC
#include <fcntl.h>
// free, realloc
#include <stdlib.h>
// strcmp, strlen, strrchr
#include <string.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "filter.h"
#include "loader.h"

static struct mtar_filter ** mtar_filter_filters = NULL;
static unsigned int mtar_filter_nb_filters = 0;

static void mtar_filter_exit(void) __attribute__((destructor));
static struct mtar_filter * mtar_filter_get(const char * module);
static const char * mtar_filter_get_module(const char * filename);


void mtar_filter_exit() {
	free(mtar_filter_filters);
	mtar_filter_filters = NULL;
}

struct mtar_filter * mtar_filter_get(const char * module) {
	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(module, mtar_filter_filters[i]->name))
			return mtar_filter_filters[i];

	if (mtar_loader_load("filter", module))
		return NULL;

	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(module, mtar_filter_filters[i]->name))
			return mtar_filter_filters[i];

	return NULL;
}

struct mtar_io_reader * mtar_filter_get_reader(const struct mtar_option * option) {
	if (option == NULL)
		return NULL;

	if (option->auto_compress && option->filename != NULL)
		return mtar_filter_get_reader3(option->filename, option);

	struct mtar_io_reader * io = NULL;
	if (option->filename != NULL)
		io = mtar_io_reader_get_file(option->filename, O_RDONLY, option);
	else
		io = mtar_io_reader_get_fd(0, option);

	return mtar_filter_get_reader2(io, option);
}

struct mtar_io_reader * mtar_filter_get_reader2(struct mtar_io_reader * io, const struct mtar_option * option) {
	if (io == NULL || option == NULL)
		return NULL;

	if (option->compress_module != NULL) {
		struct mtar_filter * filter = mtar_filter_get(option->compress_module);
		if (filter == NULL || filter->new_reader == NULL)
			return NULL;

		return filter->new_reader(io, option);
	}

	return io;
}

struct mtar_io_reader * mtar_filter_get_reader3(const char * filename, const struct mtar_option * option) {
	if (filename == NULL || option == NULL)
		return NULL;

	struct mtar_io_reader * io = mtar_io_reader_get_file(filename, O_RDONLY, option);
	const char * module = mtar_filter_get_module(filename);

	if (module != NULL) {
		struct mtar_filter * filter = mtar_filter_get(module);
		if (filter == NULL || filter->new_reader == NULL)
			return 0;

		return filter->new_reader(io, option);
	}

	return io;
}

const char * mtar_filter_get_module(const char * filename) {
	char * pos = strrchr(filename, '.');
	if (pos == NULL)
		return NULL;

	static struct {
		const char * ext;
		const char * module;
	} filenames[] = {
		{ ".bz2",  "bzip2" },
		{ ".tz2",  "bzip2" },
		{ ".tbz2", "bzip2" },
		{ ".tbz",  "bzip2" },
		{ ".gz",   "gzip" },
		{ ".tgz",  "gzip" },
		{ ".taz",  "gzip" },
		{ ".xz",   "xz" },
		{ ".txz",  "xz" },

		{ 0, 0 },
	};

	unsigned int i;
	for (i = 0; filenames[i].ext; i++)
		if (!strcmp(filenames[i].ext, pos))
			return filenames[i].module;

	return NULL;
}

struct mtar_io_writer * mtar_filter_get_writer(const struct mtar_option * option) {
	if (option == NULL)
		return NULL;

	if (option->auto_compress && option->filename != NULL)
		return mtar_filter_get_writer3(option->filename, option);

	struct mtar_io_writer * io = NULL;
	if (option->filename != NULL)
		io = mtar_io_writer_get_file(option->filename, O_RDWR | O_TRUNC, option);
	else
		io = mtar_io_writer_get_fd(1, option);

	return mtar_filter_get_writer2(io, option);
}

struct mtar_io_writer * mtar_filter_get_writer2(struct mtar_io_writer * io, const struct mtar_option * option) {
	if (io == NULL || option == NULL)
		return NULL;

	if (option->compress_module != NULL) {
		struct mtar_filter * filter = mtar_filter_get(option->compress_module);
		if (filter == NULL || filter->new_writer == NULL)
			return NULL;

		return filter->new_writer(io, option);
	}

	return io;
}

struct mtar_io_writer * mtar_filter_get_writer3(const char * filename, const struct mtar_option * option) {
	if (filename == NULL || option == NULL)
		return NULL;

	struct mtar_io_writer * io = mtar_io_writer_get_file(filename, O_RDWR | O_TRUNC, option);
	const char * module = mtar_filter_get_module(filename);

	if (module != NULL) {
		struct mtar_filter * filter = mtar_filter_get(module);
		if (filter == NULL || filter->new_writer == NULL)
			return NULL;

		return filter->new_writer(io, option);
	}

	return io;
}

void mtar_filter_register(struct mtar_filter * filter) {
	if (filter == NULL || !mtar_plugin_check(&filter->api_level))
		return;

	/**
	 * check if module has been preciously loaded
	 * or another module has the same name
	 */
	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(filter->name, mtar_filter_filters[i]->name))
			return;

	void * new_addr = realloc(mtar_filter_filters, (mtar_filter_nb_filters + 1) * sizeof(struct mtar_filter *));
	if (new_addr == NULL) {
		mtar_verbose_printf("Failed to register '%s'", filter->name);
		return;
	}

	mtar_filter_filters = new_addr;
	mtar_filter_filters[mtar_filter_nb_filters] = filter;
	mtar_filter_nb_filters++;

	mtar_loader_register_ok();
}

void mtar_filter_show_description() {
	mtar_loader_load_all("filter");
	mtar_verbose_printf("\nList of available backend filters :\n");

	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		mtar_filter_filters[i]->show_description();
}

void mtar_filter_show_version() {
	mtar_loader_load_all("filter");
	mtar_verbose_printf("List of available backend filters :\n");

	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		mtar_filter_filters[i]->show_version();

	mtar_verbose_printf("\n");
}

