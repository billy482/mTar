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
*  Last modified: Fri, 16 Sep 2011 09:10:57 +0200                       *
\***********************************************************************/

// fnmatch
#include <fnmatch.h>
// free, malloc
#include <stdlib.h>

#include <mtar/exclude.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

static int mtar_exclude_fnmatch_filter(struct mtar_exclude * ex, const char * filename);
static void mtar_exclude_fnmatch_free(struct mtar_exclude * ex);
static void mtar_exclude_fnmatch_init(void) __attribute__((constructor));
static struct mtar_exclude * mtar_exclude_fnmatch_new(const struct mtar_option * option);
static void mtar_exclude_fnmatch_show_description(void);

static struct mtar_exclude_ops mtar_exclude_fnmatch_ops = {
	.filter = mtar_exclude_fnmatch_filter,
	.free   = mtar_exclude_fnmatch_free,
};

static struct mtar_exclude_driver mtar_exclude_fnmatch_driver = {
	.name             = "fnmatch",
	.new              = mtar_exclude_fnmatch_new,
	.show_description = mtar_exclude_fnmatch_show_description,
};


int mtar_exclude_fnmatch_filter(struct mtar_exclude * ex, const char * filename) {
	unsigned int i;
	for (i = 0; i < ex->nb_excludes; i++)
		if (!fnmatch(ex->excludes[i], filename, 0))
			return 1;

	return 0;
}

void mtar_exclude_fnmatch_free(struct mtar_exclude * ex) {
	if (ex)
		free(ex);
}

void mtar_exclude_fnmatch_init() {
	mtar_exclude_register(&mtar_exclude_fnmatch_driver);
}

struct mtar_exclude * mtar_exclude_fnmatch_new(const struct mtar_option * option) {
	struct mtar_exclude * ex = malloc(sizeof(struct mtar_exclude));
	ex->excludes = option->excludes;
	ex->nb_excludes = option->nbExcludes;
	ex->ops = &mtar_exclude_fnmatch_ops;
	ex->data = 0;

	return ex;
}

void mtar_exclude_fnmatch_show_description(void) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "fnmatch based exclusion files\n");
}

