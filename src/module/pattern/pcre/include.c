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
*  Last modified: Tue, 13 Nov 2012 13:32:10 +0100                           *
\***************************************************************************/

// pcre_compile, pcre_exec, pcre_free
#include <pcre.h>
// free, malloc
#include <stdlib.h>
// strcspn, strdup, strncpy, strrchr
#include <string.h>
// lstat
#include <sys/types.h>
// lstat
#include <sys/stat.h>
// lstat
#include <unistd.h>

#include "pcre.h"

struct mtar_pattern_pcre_include {
	pcre * pattern;

	struct mtar_pattern_include * path_gen;
	char * current_dir;
	char * next_dir;
	size_t next_dir_length;
	char * next_file;
};

static void mtar_pattern_pcre_include_free(struct mtar_pattern_include * pattern);
static bool mtar_pattern_pcre_include_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option);
static bool mtar_pattern_pcre_include_match(struct mtar_pattern_include * pattern, const char * filename);
static void mtar_pattern_pcre_include_next(struct mtar_pattern_include * pattern, char ** filename);

static struct mtar_pattern_include_ops mtar_pattern_pcre_include_ops = {
	.free     = mtar_pattern_pcre_include_free,
	.has_next = mtar_pattern_pcre_include_has_next,
	.match    = mtar_pattern_pcre_include_match,
	.next     = mtar_pattern_pcre_include_next,
};


static void mtar_pattern_pcre_include_free(struct mtar_pattern_include * pattern) {
	if (pattern == NULL)
		return;

	struct mtar_pattern_pcre_include * self = pattern->data;
	pcre_free(self->pattern);
	self->pattern = NULL;

	self->path_gen->ops->free(self->path_gen);
	self->path_gen = NULL;

	free(self->current_dir);
	self->current_dir = NULL;

	free(self->next_dir);
	self->next_dir = NULL;

	free(self->next_file);
	self->next_file = NULL;

	free(self);
	free(pattern);
}

static bool mtar_pattern_pcre_include_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option) {
	struct mtar_pattern_pcre_include * self = pattern->data;

	if (self->next_dir != NULL || self->next_file != NULL)
		return true;

	while (self->path_gen->ops->has_next(self->path_gen, option)) {
		char * path = NULL;
		self->path_gen->ops->next(self->path_gen, &path);

		struct stat st;
		lstat(path, &st);

		if (strncmp(path, self->next_dir, self->next_dir_length)) {
			free(self->next_dir);
			self->next_dir = NULL;
			self->next_dir_length = 0;

			if (S_ISDIR(st.st_mode)) {
				self->next_dir = strdup(path);
				self->next_dir_length = strlen(path);
			} else {
				char * ptr = strrchr(path, '/');
				if (ptr != NULL) {
					self->next_dir_length = ptr - path;
					self->next_dir = malloc(self->next_dir_length + 1);
					strncpy(self->next_dir, path, self->next_dir_length);
					self->next_dir[self->next_dir_length] = '\0';
				}
			}
		}

		int cap[2];
		if (S_ISDIR(st.st_mode)) {
			free(self->next_dir);
			self->next_dir = path;
			self->next_dir_length = strlen(path);
		} else if (pcre_exec(self->pattern, 0, path, strlen(path), 0, 0, cap, 2) > 0) {
			self->next_file = path;
			return 1;
		} else {
			free(path);
		}
	}

	free(self->current_dir);
	self->current_dir = NULL;

	free(self->next_dir);
	self->next_dir = NULL;

	free(self->next_file);
	self->next_file = NULL;

	return false;
}

static bool mtar_pattern_pcre_include_match(struct mtar_pattern_include * pattern, const char * filename) {
	struct mtar_pattern_pcre_include * self = pattern->data;
	int cap[2];
	return pcre_exec(self->pattern, 0, filename, strlen(filename), 0, 0, cap, 2) > 0;
}

static void mtar_pattern_pcre_include_next(struct mtar_pattern_include * pattern, char ** filename) {
	struct mtar_pattern_pcre_include * self = pattern->data;

	if (self->current_dir == NULL && self->next_dir != NULL) {
		char * ptr = strchr(self->next_dir, '/');
		if (ptr != NULL) {
			self->current_dir = malloc(strlen(self->next_dir) + 1);

			size_t length = ptr - self->next_dir;
			strncpy(self->current_dir, self->next_dir, length);
			self->current_dir[length] = '\0';

			*filename = strdup(self->current_dir);
		} else {
			*filename = self->next_dir;
			self->next_dir = NULL;
			self->next_dir_length = 0;
		}
	} else if (self->current_dir != NULL) {
		ssize_t current_length = strlen(self->current_dir);

		char * ptr = strchr(self->next_dir + current_length + 1, '/');
		if (ptr != NULL) {
			ssize_t copy_length = ptr - self->next_dir - current_length;
			strncpy(self->current_dir + current_length, self->next_dir + current_length, copy_length);
			self->current_dir[current_length + copy_length] = '\0';

			*filename = strdup(self->current_dir);
		} else {
			free(self->current_dir);
			self->current_dir = NULL;

			*filename = self->next_dir;
			self->next_dir = NULL;
			self->next_dir_length = 0;
		}
	} else if (self->next_file != NULL) {
		*filename = self->next_file;
		self->next_file = NULL;
	}
}

struct mtar_pattern_include * mtar_pattern_pcre_new_include(const char * pattern, enum mtar_pattern_option option) {
	const char * error = NULL;
	int erroroffset = 0;

	int pcre_option = 0;
	if (option & mtar_pattern_option_anchored)
		pcre_option |= PCRE_ANCHORED;
	if (option & mtar_pattern_option_ignore_case)
		pcre_option |= PCRE_CASELESS;

	struct mtar_pattern_pcre_include * self = malloc(sizeof(struct mtar_pattern_pcre_include));
	self->pattern = pcre_compile(pattern, pcre_option, &error, &erroroffset, 0);

	self->path_gen = mtar_pattern_get_include(0, ".", option);

	self->current_dir = NULL;
	self->next_dir = NULL;
	self->next_dir_length = 0;
	self->next_file = NULL;

	struct mtar_pattern_include * ex = malloc(sizeof(struct mtar_pattern_include));
	ex->ops = &mtar_pattern_pcre_include_ops;
	ex->data = self;

	return ex;
}

