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
*  Last modified: Sun, 18 Sep 2011 17:37:29 +0200                       *
\***********************************************************************/

// pcre_compile, pcre_free
#include <pcre.h>
// calloc, free
#include <stdlib.h>
// strlen
#include <string.h>

#include <mtar/exclude.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

struct mtar_exclude_pcre {
	pcre ** patterns;
};

static int mtar_exclude_pcre_filter(struct mtar_exclude * ex, const char * filename);
static void mtar_exclude_pcre_free(struct mtar_exclude * ex);
static void mtar_exclude_pcre_init(void) __attribute__((constructor));
static struct mtar_exclude * mtar_exclude_pcre_new(const struct mtar_option * option);
static void mtar_exclude_pcre_show_description(void);

static struct mtar_exclude_ops mtar_exclude_pcre_ops = {
	.filter = mtar_exclude_pcre_filter,
	.free   = mtar_exclude_pcre_free,
};

static struct mtar_exclude_driver mtar_exclude_pcre_driver = {
	.name             = "pcre",
	.new              = mtar_exclude_pcre_new,
	.show_description = mtar_exclude_pcre_show_description,
};


int mtar_exclude_pcre_filter(struct mtar_exclude * ex, const char * filename) {
	struct mtar_exclude_pcre * self = ex->data;
	unsigned int i;
	int cap[2];
	for (i = 0; i < ex->nb_excludes; i++)
		if (pcre_exec(self->patterns[i], 0, filename, strlen(filename), 0, 0, cap, 2) > 0)
			return 1;

	return 0;
}

void mtar_exclude_pcre_free(struct mtar_exclude * ex) {
	if (!ex)
		return;

	struct mtar_exclude_pcre * self = ex->data;
	unsigned int i;
	for (i = 0; i < ex->nb_excludes; i++)
		pcre_free(self->patterns[i]);
	free(self->patterns);
	free(self);
	free(ex);
}

void mtar_exclude_pcre_init() {
	mtar_exclude_register(&mtar_exclude_pcre_driver);
}

struct mtar_exclude * mtar_exclude_pcre_new(const struct mtar_option * option) {
	struct mtar_exclude_pcre * self = malloc(sizeof(struct mtar_exclude_pcre));
	self->patterns = calloc(option->nb_excludes, sizeof(pcre *));

	unsigned int i;
	const char * error = 0;
	int erroroffset = 0;
	for (i = 0; i < option->nb_excludes; i++)
		self->patterns[i] = pcre_compile(option->excludes[i], 0, &error, &erroroffset, 0);

	struct mtar_exclude * ex = malloc(sizeof(struct mtar_exclude));
	ex->excludes = option->excludes;
	ex->nb_excludes = option->nb_excludes;
	ex->ops = &mtar_exclude_pcre_ops;
	ex->data = self;

	return ex;
}

void mtar_exclude_pcre_show_description(void) {
	mtar_verbose_printf("pcre powered exclusion files\n");
}

