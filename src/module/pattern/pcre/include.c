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
*  Last modified: Tue, 25 Oct 2011 12:18:52 +0200                           *
\***************************************************************************/

// pcre_compile, pcre_exec, pcre_free
#include <pcre.h>
// free, malloc
#include <stdlib.h>
// strcspn, strdup, strncpy, strrchr
#include <string.h>

#include "common.h"

struct mtar_pattern_pcre_include {
	pcre * pattern;
	enum mtar_pattern_option option;

	struct mtar_pattern_include * path_gen;
	char next_file[256];
	char next_dir[256];
	size_t dir_length;
};

static void mtar_pattern_pcre_include_free(struct mtar_pattern_include * pattern);
static int mtar_pattern_pcre_include_has_next(struct mtar_pattern_include * pattern);
static void mtar_pattern_pcre_include_next(struct mtar_pattern_include * pattern, char * filename, size_t length);

static struct mtar_pattern_include_ops mtar_pattern_pcre_include_ops = {
	.free     = mtar_pattern_pcre_include_free,
	.has_next = mtar_pattern_pcre_include_has_next,
	.next     = mtar_pattern_pcre_include_next,
};


void mtar_pattern_pcre_include_free(struct mtar_pattern_include * pattern) {
	if (!pattern)
		return;

	struct mtar_pattern_pcre_include * self = pattern->data;
	if (self->pattern)
		pcre_free(self->pattern);

	if (self->path_gen)
		self->path_gen->ops->free(self->path_gen);
	self->path_gen = 0;

	free(self);
	free(pattern);
}

int mtar_pattern_pcre_include_has_next(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_pcre_include * self = pattern->data;

	if (self->dir_length > 0) {
		char * ptr = strchr(self->next_file + self->dir_length, '/');
		if (ptr) {
			self->dir_length = ptr - self->next_file;
			strncpy(self->next_dir, self->next_file, self->dir_length);
			self->next_dir[self->dir_length] = '\0';
			self->dir_length++;
		} else
			self->dir_length = 0;

		return 1;
	}

	while (self->path_gen->ops->has_next(self->path_gen)) {
		self->path_gen->ops->next(self->path_gen, self->next_file, 256);

		int cap[2];
		if (pcre_exec(self->pattern, 0, self->next_file, strlen(self->next_file), 0, 0, cap, 2) > 0) {
			int i;
			for (i = 0; self->next_dir[i] == self->next_file[i]; i++);
			char * ptr = strchr(self->next_file + i + 1, '/');
			if (ptr) {
				self->dir_length = ptr - self->next_file;
				strncpy(self->next_dir, self->next_file, self->dir_length);
				self->next_dir[self->dir_length] = '\0';
				self->dir_length++;
			} else
				self->dir_length = 0;
			return 1;
		}
	}

	return 0;
}

void mtar_pattern_pcre_include_next(struct mtar_pattern_include * pattern, char * filename, size_t length) {
	struct mtar_pattern_pcre_include * self = pattern->data;
	if (self->dir_length > 0)
		strncpy(filename, self->next_dir, length);
	else
		strncpy(filename, self->next_file, length);
}

struct mtar_pattern_include * mtar_pattern_pcre_new_include(const char * pattern, enum mtar_pattern_option option) {
	const char * error = 0;
	int erroroffset = 0;

	struct mtar_pattern_pcre_include * self = malloc(sizeof(struct mtar_pattern_pcre_include));
	self->pattern = pcre_compile(pattern, 0, &error, &erroroffset, 0);
	self->option = option;

	self->path_gen = 0;
	size_t pos = strcspn(pattern, "*?{[");

	if (pos > 0) {
		strncpy(self->next_file, pattern, pos);
		self->next_file[pos] = '\0';

		char * last_slash = strrchr(self->next_file, '/');

		if (last_slash) {
			*last_slash = '\0';
			self->path_gen = mtar_pattern_get_include(0, self->next_file, option);
		} else {
			self->path_gen = mtar_pattern_get_include(0, ".", option);
		}
	} else {
		self->path_gen = mtar_pattern_get_include(0, ".", option);
	}

	*self->next_file = '\0';
	*self->next_dir = '\0';
	self->dir_length = 0;

	struct mtar_pattern_include * ex = malloc(sizeof(struct mtar_pattern_include));
	ex->ops = &mtar_pattern_pcre_include_ops;
	ex->data = self;

	return ex;
}
