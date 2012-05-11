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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Sat, 12 May 2012 00:19:45 +0200                           *
\***************************************************************************/

// versionsort
#define _GNU_SOURCE

// scandir
#include <dirent.h>
// fnmatch
#include <fnmatch.h>
// free, realloc
#include <stdlib.h>
// memmove, strcmp, strdup, strlen, strrchr, strstr
#include <string.h>
// lstat, stat
#include <sys/types.h>
// lstat, stat
#include <sys/stat.h>
// access, lstat, stat
#include <unistd.h>

#include <mtar/filter.h>
#include <mtar/option.h>
#include <mtar/readline.h>
#include <mtar/verbose.h>

#include "loader.h"
#include "pattern.h"

struct mtar_pattern_include_private {
	char * path;
	enum mtar_pattern_option option;

	char * current_path;

	struct mtar_pattern_include_node {
		struct dirent ** nl;
		struct dirent * nl_next;
		int nb_nl;
		int index_nl;

		struct mtar_pattern_include_node * next;
		struct mtar_pattern_include_node * previous;
	} * first, * last;

	enum {
		MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED,
		MTAR_PATTERN_INCLUDE_PRIVATE_HAS_NEXT,
		MTAR_PATTERN_INCLUDE_PRIVATE_INIT,
	} status;
};

static struct mtar_pattern_driver ** mtar_pattern_drivers = 0;
static unsigned int mtar_pattern_nb_drivers = 0;

static void mtar_pattern_exit(void) __attribute__((destructor));
static int mtar_pattern_include_filter(const struct dirent * file);
static void mtar_pattern_include_private_free(struct mtar_pattern_include * pattern);
static int mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option);
static void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char ** filename);
static struct mtar_pattern_include * mtar_pattern_include_private_new(const char * pattern, enum mtar_pattern_option option);

static struct mtar_pattern_include_ops mtar_pattern_include_private_ops = {
	.free     = mtar_pattern_include_private_free,
	.has_next = mtar_pattern_include_private_has_next,
	.next     = mtar_pattern_include_private_next,
};


struct mtar_pattern_exclude ** mtar_pattern_add_exclude(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option) {
	if (!nb_patterns || !engine || !pattern)
		return patterns;

	patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern_exclude *));
	patterns[*nb_patterns] = mtar_pattern_get_exclude(engine, pattern, option);
	(*nb_patterns)++;

	return patterns;
}

struct mtar_pattern_include ** mtar_pattern_add_include(struct mtar_pattern_include ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option) {
	if (!nb_patterns || !engine || !pattern)
		return patterns;

	patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern *));
	patterns[*nb_patterns] = mtar_pattern_get_include(engine, pattern, option);
	(*nb_patterns)++;

	return patterns;
}

struct mtar_pattern_exclude ** mtar_pattern_add_exclude_from_file(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, enum mtar_pattern_option option, const char * filename, struct mtar_option * op) {
	if (!nb_patterns || !engine || !filename)
		return patterns;

	struct mtar_io_in * file = mtar_filter_get_in3(filename, op);
	if (!file)
		return patterns;

	struct mtar_readline * rl = mtar_readline_new(file, op->delimiter);
	char * line;
	while ((line = mtar_readline_getline(rl))) {
		if (strlen(line) == 0) {
			free(line);
			continue;
		}

		patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern *));
		patterns[*nb_patterns] = mtar_pattern_get_exclude(engine, line, option);
		(*nb_patterns)++;
	}
	mtar_readline_free(rl);

	return patterns;
}

struct mtar_pattern_include ** mtar_pattern_add_include_from_file(struct mtar_pattern_include ** patterns, unsigned int * nb_patterns, char * engine, enum mtar_pattern_option option, const char * filename, struct mtar_option * op) {
	if (!nb_patterns || !engine || !filename)
		return patterns;

	struct mtar_io_in * file = mtar_filter_get_in3(filename, op);
	if (!file)
		return patterns;

	struct mtar_readline * rl = mtar_readline_new(file, op->delimiter);
	char * line;
	while ((line = mtar_readline_getline(rl))) {
		if (strlen(line) == 0) {
			free(line);
			continue;
		}

		patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern *));
		patterns[*nb_patterns] = mtar_pattern_get_include(engine, line, option);
		(*nb_patterns)++;
	}
	mtar_readline_free(rl);

	return patterns;
}

struct mtar_pattern_tag * mtar_pattern_add_tag(struct mtar_pattern_tag * tags, unsigned int * nb_tags, char * tag, enum mtar_pattern_tag_option option) {
	tags = realloc(tags, (*nb_tags + 1) * sizeof(struct mtar_pattern_tag));
	tags[*nb_tags].tag = tag;
	tags[*nb_tags].option = option;
	(*nb_tags)++;
	return tags;
}

void mtar_pattern_exit() {
	if (mtar_pattern_nb_drivers > 0)
		free(mtar_pattern_drivers);
	mtar_pattern_drivers = 0;
	mtar_pattern_nb_drivers = 0;
}

struct mtar_pattern_exclude * mtar_pattern_get_exclude(const char * engine, const char * pattern, enum mtar_pattern_option option) {
	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_exclude(pattern, option);
	if (mtar_loader_load("pattern", engine))
		return 0;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_exclude(pattern, option);
	return 0;
}

struct mtar_pattern_include * mtar_pattern_get_include(const char * engine, const char * pattern, enum mtar_pattern_option option) {
	if (!access(pattern, F_OK | R_OK))
		return mtar_pattern_include_private_new(pattern, option);

	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_include(pattern, option);
	if (mtar_loader_load("pattern", engine))
		return 0;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_include(pattern, option);
	return 0;
}

int mtar_pattern_include_filter(const struct dirent * file) {
	return !strcmp(".", file->d_name) || !strcmp("..", file->d_name) ? 0 : 1;
}

void mtar_pattern_include_private_free(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_include_private * self = pattern->data;

	if (self->path)
		free(self->path);
	self->path = 0;

	struct mtar_pattern_include_node * ptr;
	for (ptr = self->first; ptr; ptr = ptr->next) {
		if (ptr->previous)
			free(ptr->previous);

		while (ptr->index_nl < ptr->nb_nl) {
			free(ptr->nl_next);
			ptr->nl_next = ptr->nl[++ptr->index_nl];
		}
		free(ptr->nl);
	}

	free(self);
	free(pattern);
}

int mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option) {
	struct mtar_pattern_include_private * self = pattern->data;

	if (self->current_path)
		free(self->current_path);
	self->current_path = 0;

	size_t length;
	struct mtar_pattern_include_node * ptr, * cnode = self->last;
	struct stat st;

	switch (self->status) {
		case MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED:
			return 0;

		case MTAR_PATTERN_INCLUDE_PRIVATE_HAS_NEXT:
			if (!cnode && self->option & MTAR_PATTERN_OPTION_RECURSION) {
				cnode = malloc(sizeof(struct mtar_pattern_include_private));

				cnode->nl = 0;
				cnode->nl_next = 0;
				cnode->next = cnode->previous = 0;
				cnode->index_nl = 0;
				cnode->next = cnode->previous = 0;

				cnode->nb_nl = scandir(self->path, &cnode->nl, mtar_pattern_include_filter, versionsort);

				if (cnode->nb_nl > 0) {
					cnode->nl_next = *cnode->nl;

					self->first = self->last = cnode;
				} else {
					free(cnode);
					self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
					return 0;
				}
			}

			while (cnode) {
				length = strlen(self->path);
				for (ptr = self->first; ptr; ptr = ptr->next)
					length += strlen(ptr->nl_next->d_name) + 1;

				self->current_path = malloc(length + 1);
				strcpy(self->current_path, self->path);
				for (ptr = self->first; ptr; ptr = ptr->next) {
					strcat(self->current_path, "/");
					strcat(self->current_path, ptr->nl_next->d_name);
				}

				if (lstat(self->current_path, &st)) {
					free(self->current_path);
					self->current_path = 0;

					self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
					return 0;
				}

				if (mtar_pattern_match(option, self->current_path)) {
					free(self->current_path);
					self->current_path = 0;
				} else if (S_ISDIR(st.st_mode)) {
					struct mtar_pattern_include_node * csubnode = malloc(sizeof(struct mtar_pattern_include_private));
					csubnode->next = csubnode->previous = 0;

					csubnode->nl = 0;
					csubnode->nl_next = 0;
					csubnode->index_nl = 0;

					csubnode->nb_nl = scandir(self->current_path, &csubnode->nl, mtar_pattern_include_filter, versionsort);

					if (csubnode->nb_nl > 0) {
						csubnode->nl_next = *csubnode->nl;

						self->last = cnode->next = csubnode;
						csubnode->previous = cnode;

						return 1;
					}

					free(csubnode);
				}

				do {
					free(cnode->nl_next);
					cnode->nl_next = cnode->nl[++cnode->index_nl];

					if (self->current_path && cnode->index_nl < cnode->nb_nl)
						return 1;

					if (cnode->index_nl < cnode->nb_nl)
						break;

					free(cnode->nl);
					ptr = cnode->previous;

					if (ptr)
						ptr->next = 0;

					free(cnode);
					self->last = cnode = ptr;

					if (!self->last)
						self->first = 0;
				} while (cnode);
			}

			self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
			return self->current_path ? 1 : 0;

		case MTAR_PATTERN_INCLUDE_PRIVATE_INIT:
			if (lstat(self->path, &st)) {
				self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
				return 0;
			}

			if (mtar_pattern_match(option, self->path)) {
				self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
				return 0;
			}

			self->current_path = strdup(self->path);
			self->status = S_ISDIR(st.st_mode) ? MTAR_PATTERN_INCLUDE_PRIVATE_HAS_NEXT : MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
			break;
	}

	return 1;
}

void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char ** filename) {
	struct mtar_pattern_include_private * self = pattern->data;

	switch (self->status) {
		case MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED:
		case MTAR_PATTERN_INCLUDE_PRIVATE_HAS_NEXT:
			if (self->current_path)
				*filename = strdup(self->current_path);
			break;

		case MTAR_PATTERN_INCLUDE_PRIVATE_INIT:
			break;
	}
}

struct mtar_pattern_include * mtar_pattern_include_private_new(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_include_private * self = malloc(sizeof(struct mtar_pattern_include_private));
	self->path = strdup(pattern);
	self->option = option;
	self->current_path = 0;
	self->first = self->last = 0;
	self->status = MTAR_PATTERN_INCLUDE_PRIVATE_INIT;

	// remove double '/'
	char * ptr = self->path;
	while ((ptr = strstr(ptr, "//"))) {
		memmove(ptr, ptr + 1, strlen(ptr));
	}

	// remove trailing '/'
	size_t length = strlen(pattern);
	while (length > 0 && self->path[length - 1] == '/') {
		length--;
		self->path[length] = '\0';
	}

	struct mtar_pattern_include * pttrn = malloc(sizeof(struct mtar_pattern_include));
	pttrn->ops = &mtar_pattern_include_private_ops;
	pttrn->data = self;

	return pttrn;
}

int mtar_pattern_match(const struct mtar_option * option, const char * filename) {
	if (!option || !filename)
		return 1;

	unsigned int i;
	for (i = 0; i < option->nb_excludes; i++)
		if (option->excludes[i]->ops->match(option->excludes[i], filename))
			return 1;

	const char * file = strrchr(filename, '/');
	if (file)
		file++;
	else
		file = filename;

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

	if (option->nb_exclude_tags) {
		char path[256];
		size_t length;
		struct stat st;
		stat(filename, &st);

		if (S_ISDIR(st.st_mode)) {
			strcpy(path, filename);
			length = strlen(path);
			if (path[length - 1] != '/') {
				strcat(path, "/");
				length++;
			}

			for (i = 0; i < option->nb_exclude_tags; i++) {
				strcpy(path + length, option->exclude_tags[i].tag);

				if (!access(path, F_OK)) {
					switch (option->exclude_tags[i].option) {
						case MTAR_PATTERN_TAG:
						case MTAR_PATTERN_TAG_UNDER:
							continue;

						case MTAR_PATTERN_TAG_ALL:
							return 1;
					}
				}
			}

			return 1;
		}

		for (i = 0; i < option->nb_exclude_tags; i++) {
			if (!strcmp(file, option->exclude_tags[i].tag)) {
				switch (option->exclude_tags[i].option) {
					case MTAR_PATTERN_TAG:
						continue;

					case MTAR_PATTERN_TAG_ALL:
					case MTAR_PATTERN_TAG_UNDER:
						return 1;
				}
			}
		}

		length = file - filename;
		if (length > 0)
			strncpy(path, filename, length);

		for (i = 0; i < option->nb_exclude_tags; i++) {
			strcpy(path + length, option->exclude_tags[i].tag);

			if (strcmp(path, filename) && !access(path, F_OK))
				return 1;
		}
	}

	return 0;
}

void mtar_pattern_register(struct mtar_pattern_driver * driver) {
	if (!driver || driver->api_version != MTAR_PATTERN_API_VERSION)
		return;

	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(driver->name, mtar_pattern_drivers[i]->name))
			return;

	mtar_pattern_drivers = realloc(mtar_pattern_drivers, (mtar_pattern_nb_drivers + 1) * sizeof(struct mtar_pattern_driver *));
	mtar_pattern_drivers[mtar_pattern_nb_drivers] = driver;
	mtar_pattern_nb_drivers++;

	mtar_loader_register_ok();
}

void mtar_pattern_show_description() {
	mtar_loader_load_all("pattern");

	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		mtar_pattern_drivers[i]->show_description();
}

void mtar_pattern_show_version() {
	mtar_loader_load_all("pattern");
	mtar_verbose_printf("List of available backend patterns :\n");

	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		mtar_pattern_drivers[i]->show_version();

	mtar_verbose_printf("\n");
}

