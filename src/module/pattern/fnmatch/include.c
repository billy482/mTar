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
*  Last modified: Tue, 18 Oct 2011 18:57:00 +0200                           *
\***************************************************************************/

// fnmatch
#include <fnmatch.h>
// free, malloc
#include <stdlib.h>
// strcspn, strdup, strncpy, strrchr
#include <string.h>

#include "common.h"

struct mtar_pattern_fnmatch_include {
	char * pattern;
	enum mtar_pattern_option option;

	struct mtar_pattern_include * path_gen;
	char next_path[256];
};

static void mtar_pattern_fnmatch_include_free(struct mtar_pattern_include * pattern);
static int mtar_pattern_fnmatch_include_has_next(struct mtar_pattern_include * pattern);
static void mtar_pattern_fnmatch_include_next(struct mtar_pattern_include * pattern, char * filename, size_t length);

static struct mtar_pattern_include_ops mtar_pattern_fnmatch_include_ops = {
	.free     = mtar_pattern_fnmatch_include_free,
	.has_next = mtar_pattern_fnmatch_include_has_next,
	.next     = mtar_pattern_fnmatch_include_next,
};


void mtar_pattern_fnmatch_include_free(struct mtar_pattern_include * pattern) {
	if (!pattern)
		return;

	struct mtar_pattern_fnmatch_include * self = pattern->data;
	if (self->pattern)
		free(self->pattern);

	if (self->path_gen)
		self->path_gen->ops->free(self->path_gen);
	self->path_gen = 0;

	free(self);
	free(pattern);
}

int mtar_pattern_fnmatch_include_has_next(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_fnmatch_include * self = pattern->data;

	while (self->path_gen->ops->has_next(self->path_gen)) {
		self->path_gen->ops->next(self->path_gen, self->next_path, 256);

		if (!fnmatch(self->pattern, self->next_path, 0))
			return 1;
	}

	return 0;
}

void mtar_pattern_fnmatch_include_next(struct mtar_pattern_include * pattern, char * filename, size_t length) {
	struct mtar_pattern_fnmatch_include * self = pattern->data;
	strncpy(filename, self->next_path, length);
}

struct mtar_pattern_include * mtar_pattern_fnmatch_new_include(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_fnmatch_include * self = malloc(sizeof(struct mtar_pattern_fnmatch_include));
	self->pattern = strdup(pattern);
	self->option = option;

	self->path_gen = 0;
	size_t pos = strcspn(pattern, "*?{[");

	if (pos > 0) {
		strncpy(self->next_path, pattern, pos);
		self->next_path[pos] = '\0';

		char * last_slash = strrchr(self->next_path, '/');

		if (last_slash) {
			*last_slash = '\0';
			self->path_gen = mtar_pattern_get_include(0, self->next_path, option);
		} else {
			self->path_gen = mtar_pattern_get_include(0, ".", option);
		}
	} else {
		self->path_gen = mtar_pattern_get_include(0, ".", option);
	}

	*self->next_path = '\0';

	struct mtar_pattern_include * ex = malloc(sizeof(struct mtar_pattern_include));
	ex->ops = &mtar_pattern_fnmatch_include_ops;
	ex->data = self;

	return ex;
}

