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
*  Last modified: Fri, 16 Sep 2011 11:59:01 +0200                       *
\***********************************************************************/

// fnmatch
#include <fnmatch.h>
// free, realloc
#include <stdlib.h>
// strcmp, strlen, strrchr
#include <string.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "exclude.h"
#include "loader.h"

static struct mtar_exclude_driver ** mtar_exclude_exs = 0;
static unsigned int mtar_exclude_nbExs = 0;

static void mtar_exclude_exit(void) __attribute__((destructor));


int mtar_exclude_filter(const char * filename, const struct mtar_option * option) {
	if (!option)
		return 0;

	const char * file = strrchr(filename, '/');
	if (file)
		file++;
	else
		file = filename;

	unsigned int i;
	if (option->exclude_option & MTAR_EXCLUDE_OPTION_BACKUP) {
		static const char * patterns[] = { ".#*", "*~", "#*#", 0 };

		for (i = 0; patterns[i]; i++)
			if (!fnmatch(patterns[i], file, 0))
				return 1;
	}

	if (option->exclude_option & MTAR_EXCLUDE_OPTION_VCS) {
		static const char * patterns[] = {
			"CVS", "RCS", "SCCS", ".git", ".gitignore", ".cvsignore", ".svn",
			".arch-ids", "{arch}", "=RELEASE-ID", "=meta-update", "=update",
			".bzr", ".bzrignore", ".bzrtags", ".hg", ".hgignore", ".hgrags",
			"_darcs", 0
		};

		for (i = 0; patterns[i]; i++)
			if (!strcmp(patterns[i], file))
				return 1;
	}

	return 0;
}

void mtar_exclude_exit() {
	if (mtar_exclude_nbExs > 0)
		free(mtar_exclude_exs);
	mtar_exclude_exs = 0;
}

struct mtar_exclude * mtar_exclude_get(const struct mtar_option * option) {
	if (option->nbExcludes < 1)
		return 0;

	unsigned int i;
	for (i = 0; i < mtar_exclude_nbExs; i++)
		if (!strcmp(option->exclude_engine, mtar_exclude_exs[i]->name))
			return mtar_exclude_exs[i]->new(option);
	if (mtar_loader_load("exclude", option->exclude_engine))
		return 0;
	for (i = 0; i < mtar_exclude_nbExs; i++)
		if (!strcmp(option->exclude_engine, mtar_exclude_exs[i]->name))
			return mtar_exclude_exs[i]->new(option);

	return 0;
}

void mtar_exclude_register(struct mtar_exclude_driver * driver) {
	if (!driver)
		return;

	unsigned int i;
	for (i = 0; i < mtar_exclude_nbExs; i++)
		if (!strcmp(driver->name, mtar_exclude_exs[i]->name))
			return;

	mtar_exclude_exs = realloc(mtar_exclude_exs, (mtar_exclude_nbExs + 1) * sizeof(struct mtar_exclude_driver *));
	mtar_exclude_exs[mtar_exclude_nbExs] = driver;
	mtar_exclude_nbExs++;

	mtar_loader_register_ok();
}

void mtar_exclude_show_description() {
	mtar_loader_loadAll("exclude");

	unsigned int i, length = 0;
	for (i = 0; i < mtar_exclude_nbExs; i++) {
		unsigned int ll = strlen(mtar_exclude_exs[i]->name);
		if (ll > length)
			length = ll;
	}

	for (i = 0; i < mtar_exclude_nbExs; i++) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    %-*s : ", length, mtar_exclude_exs[i]->name);
		mtar_exclude_exs[i]->show_description();
	}
}

