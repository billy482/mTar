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
*  Last modified: Tue, 13 Sep 2011 09:55:59 +0200                       *
\***********************************************************************/

// O_RDONLY, O_RDWR
#include <fcntl.h>
// free, realloc
#include <stdlib.h>
// strcmp, strlen, strrchr
#include <string.h>

#include "filter.h"
#include "loader.h"
#include "option.h"
#include "verbose.h"

static struct mtar_filter ** mtar_filter_filters = 0;
static unsigned int mtar_filter_nb_filters = 0;

static void mtar_filter_exit(void) __attribute__((destructor));
static struct mtar_filter * mtar_filter_get(const char * module);
static const char * mtar_filter_get_module(const char * filename);


void mtar_filter_exit() {
	if (mtar_filter_nb_filters > 0)
		free(mtar_filter_filters);
	mtar_filter_filters = 0;
}

struct mtar_filter * mtar_filter_get(const char * module) {
	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(module, mtar_filter_filters[i]->name))
			return mtar_filter_filters[i];
	if (mtar_loader_load("filter", module))
		return 0;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(module, mtar_filter_filters[i]->name))
			return mtar_filter_filters[i];
	return 0;
}

struct mtar_io_in * mtar_filter_get_in(const struct mtar_option * option) {
	if (!option)
		return 0;

	struct mtar_io_in * io = 0;
	if (option->filename)
		io = mtar_io_in_get_file(option->filename, O_RDONLY, option);
	else
		io = mtar_io_in_get_fd(0, O_RDONLY, option);

	return mtar_filter_get_in2(io, option);
}

struct mtar_io_in * mtar_filter_get_in2(struct mtar_io_in * io, const struct mtar_option * option) {
	if (!io || !option)
		return 0;

	if (option->compress_module) {
		struct mtar_filter * filter = mtar_filter_get(option->compress_module);
		if (!filter)
			return 0;

		return filter->new_in(io, option);
	}
	return io;
}

struct mtar_io_in * mtar_filter_get_in3(const char * filename, const struct mtar_option * option) {
	if (!filename || !option)
		return 0;

	struct mtar_io_in * io = mtar_io_in_get_file(filename, O_RDONLY, option);
	const char * module = mtar_filter_get_module(filename);

	if (module) {
		struct mtar_filter * filter = mtar_filter_get(module);
		if (!filter)
			return 0;

		return filter->new_in(io, option);
	}

	return io;
}

const char * mtar_filter_get_module(const char * filename) {
	char * pos = strrchr(filename, '.');
	if (!pos)
		return 0;

	static struct {
		const char * ext;
		const char * module;
	} filenames[] = {
		{ ".bz2",  "bzip2" },
		{ ".tz2",  "bzip2" },
		{ ".tbz2", "bzip2" },
		{ ".tbz",  "bzip2" },
		{ ".gz",   "gzip"  },
		{ ".tgz",  "gzip"  },
		{ ".taz",  "gzip"  },

		{ 0, 0 },
	};

	unsigned int i;
	for (i = 0; filenames[i].ext; i++)
		if (!strcmp(filenames[i].ext, pos))
			return filenames[i].module;

	return 0;
}

struct mtar_io_out * mtar_filter_get_out(const struct mtar_option * option) {
	if (!option)
		return 0;

	struct mtar_io_out * io = 0;
	if (option->filename)
		io = mtar_io_out_get_file(option->filename, O_RDWR | O_TRUNC, option);
	else
		io = mtar_io_out_get_fd(1, O_RDWR, option);

	return mtar_filter_get_out2(io, option);
}

struct mtar_io_out * mtar_filter_get_out2(struct mtar_io_out * io, const struct mtar_option * option) {
	if (!io || !option)
		return 0;

	if (option->compress_module) {
		struct mtar_filter * filter = mtar_filter_get(option->compress_module);
		if (!filter)
			return 0;

		return filter->new_out(io, option);
	}
	return io;
}

struct mtar_io_out * mtar_filter_get_out3(const char * filename, const struct mtar_option * option) {
	if (!filename || !option)
		return 0;

	struct mtar_io_out * io = mtar_io_out_get_file(filename, O_RDWR | O_TRUNC, option);
	const char * module = mtar_filter_get_module(filename);

	if (module) {
		struct mtar_filter * filter = mtar_filter_get(module);
		if (!filter)
			return 0;

		return filter->new_out(io, option);
	}

	return io;
}

void mtar_filter_register(struct mtar_filter * filter) {
	if (!filter)
		return;

	unsigned int i;
	for (i = 0; i < mtar_filter_nb_filters; i++)
		if (!strcmp(filter->name, mtar_filter_filters[i]->name))
			return;

	mtar_filter_filters = realloc(mtar_filter_filters, (mtar_filter_nb_filters + 1) * sizeof(struct mtar_filter *));
	mtar_filter_filters[mtar_filter_nb_filters] = filter;
	mtar_filter_nb_filters++;

	mtar_loader_register_ok();
}

void mtar_filter_show_description() {
	mtar_loader_loadAll("filter");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "\nList of available backend filters :\n");

	unsigned int i, length = 0;
	for (i = 0; i < mtar_filter_nb_filters; i++) {
		unsigned int ll = strlen(mtar_filter_filters[i]->name);
		if (ll > length)
			length = ll;
	}

	for (i = 0; i < mtar_filter_nb_filters; i++) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    %-*s : ", length, mtar_filter_filters[i]->name);
		mtar_filter_filters[i]->show_description();
	}
}

