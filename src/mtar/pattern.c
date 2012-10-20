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
*  Last modified: Sat, 20 Oct 2012 11:02:17 +0200                           *
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
		mtar_pattern_include_status_finished,
		mtar_pattern_include_status_has_next,
		mtar_pattern_include_status_init,
	} status;
};

static struct mtar_pattern_driver ** mtar_pattern_drivers = NULL;
static unsigned int mtar_pattern_nb_drivers = 0;

static void mtar_pattern_exit(void) __attribute__((destructor));
static int mtar_pattern_include_filter(const struct dirent * file);
static void mtar_pattern_include_private_free(struct mtar_pattern_include * pattern);
static bool mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option);
static void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char ** filename);
static struct mtar_pattern_include * mtar_pattern_include_private_new(const char * pattern, enum mtar_pattern_option option);

static struct mtar_pattern_include_ops mtar_pattern_include_private_ops = {
	.free     = mtar_pattern_include_private_free,
	.has_next = mtar_pattern_include_private_has_next,
	.next     = mtar_pattern_include_private_next,
};


struct mtar_pattern_exclude ** mtar_pattern_add_exclude(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option) {
	if (nb_patterns == NULL || engine == NULL || pattern == NULL)
		return patterns;

	patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern_exclude *));
	patterns[*nb_patterns] = mtar_pattern_get_exclude(engine, pattern, option);
	(*nb_patterns)++;

	return patterns;
}

struct mtar_pattern_include ** mtar_pattern_add_include(struct mtar_pattern_include ** patterns, unsigned int * nb_patterns, char * engine, char * pattern, enum mtar_pattern_option option) {
	if (nb_patterns == NULL || engine == NULL || pattern == NULL)
		return patterns;

	patterns = realloc(patterns, (*nb_patterns + 1) * sizeof(struct mtar_pattern *));
	patterns[*nb_patterns] = mtar_pattern_get_include(engine, pattern, option);
	(*nb_patterns)++;

	return patterns;
}

struct mtar_pattern_exclude ** mtar_pattern_add_exclude_from_file(struct mtar_pattern_exclude ** patterns, unsigned int * nb_patterns, char * engine, enum mtar_pattern_option option, const char * filename, struct mtar_option * op) {
	if (nb_patterns == NULL || engine == NULL || filename == NULL)
		return patterns;

	struct mtar_io_in * file = mtar_filter_get_in3(filename, op);
	if (file == NULL)
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
	if (nb_patterns == NULL || engine == NULL || filename == NULL)
		return patterns;

	struct mtar_io_in * file = mtar_filter_get_in3(filename, op);
	if (file == NULL)
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
	mtar_pattern_drivers = NULL;
	mtar_pattern_nb_drivers = 0;
}

struct mtar_pattern_exclude * mtar_pattern_get_exclude(const char * engine, const char * pattern, enum mtar_pattern_option option) {
	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_exclude(pattern, option);

	if (mtar_loader_load("pattern", engine))
		return NULL;

	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_exclude(pattern, option);

	return NULL;
}

struct mtar_pattern_include * mtar_pattern_get_include(const char * engine, const char * pattern, enum mtar_pattern_option option) {
	if (!access(pattern, F_OK | R_OK))
		return mtar_pattern_include_private_new(pattern, option);

	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_include(pattern, option);

	if (mtar_loader_load("pattern", engine))
		return NULL;

	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(engine, mtar_pattern_drivers[i]->name))
			return mtar_pattern_drivers[i]->new_include(pattern, option);

	return NULL;
}

int mtar_pattern_include_filter(const struct dirent * file) {
	if (file->d_name[0] != '.')
		return 1;

	if (file->d_name[1] == '\0')
		return 0;

	return file->d_name[1] != '.' || file->d_name[2] != '\0';
}

void mtar_pattern_include_private_free(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_include_private * self = pattern->data;

	free(self->path);
	self->path = NULL;

	struct mtar_pattern_include_node * ptr;
	for (ptr = self->first; ptr; ptr = ptr->next) {
		free(ptr->previous);
		ptr->previous = NULL;

		while (ptr->index_nl < ptr->nb_nl) {
			free(ptr->nl_next);
			ptr->nl_next = ptr->nl[++ptr->index_nl];
		}
		free(ptr->nl);
	}

	free(self);
	free(pattern);
}

bool mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option) {
	struct mtar_pattern_include_private * self = pattern->data;

	free(self->current_path);
	self->current_path = NULL;

	size_t length;
	struct mtar_pattern_include_node * ptr, * cnode = self->last;
	struct stat st;

	switch (self->status) {
		case mtar_pattern_include_status_finished:
			return false;

		case mtar_pattern_include_status_has_next:
			if (cnode == NULL && self->option & mtar_pattern_option_recursion) {
				cnode = malloc(sizeof(struct mtar_pattern_include_private));

				cnode->nl = NULL;
				cnode->nl_next = NULL;
				cnode->next = cnode->previous = NULL;
				cnode->index_nl = 0;
				cnode->next = cnode->previous = NULL;

				cnode->nb_nl = scandir(self->path, &cnode->nl, mtar_pattern_include_filter, versionsort);

				if (cnode->nb_nl > 0) {
					cnode->nl_next = *cnode->nl;

					self->first = self->last = cnode;
				} else {
					free(cnode);
					self->status = mtar_pattern_include_status_finished;
					return false;
				}
			}

			while (cnode != NULL) {
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

					self->status = mtar_pattern_include_status_finished;
					return false;
				}

				if (mtar_pattern_match(option, self->current_path)) {
					free(self->current_path);
					self->current_path = NULL;
				} else if (S_ISDIR(st.st_mode)) {
					struct mtar_pattern_include_node * csubnode = malloc(sizeof(struct mtar_pattern_include_private));
					csubnode->next = csubnode->previous = NULL;

					csubnode->nl = NULL;
					csubnode->nl_next = NULL;
					csubnode->index_nl = 0;

					csubnode->nb_nl = scandir(self->current_path, &csubnode->nl, mtar_pattern_include_filter, versionsort);

					if (csubnode->nb_nl > 0) {
						csubnode->nl_next = *csubnode->nl;

						self->last = cnode->next = csubnode;
						csubnode->previous = cnode;

						return true;
					}

					free(csubnode);
				}

				do {
					free(cnode->nl_next);
					cnode->nl_next = cnode->nl[++cnode->index_nl];

					if (self->current_path && cnode->index_nl < cnode->nb_nl)
						return true;

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
				} while (cnode != NULL);
			}

			self->status = mtar_pattern_include_status_finished;
			return self->current_path != NULL;

		case mtar_pattern_include_status_init:
			if (lstat(self->path, &st)) {
				self->status = mtar_pattern_include_status_finished;
				return false;
			}

			if (mtar_pattern_match(option, self->path)) {
				self->status = mtar_pattern_include_status_finished;
				return false;
			}

			self->current_path = strdup(self->path);
			self->status = S_ISDIR(st.st_mode) ? mtar_pattern_include_status_has_next : mtar_pattern_include_status_finished;
			break;
	}

	return true;
}

void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char ** filename) {
	struct mtar_pattern_include_private * self = pattern->data;

	switch (self->status) {
		case mtar_pattern_include_status_finished:
		case mtar_pattern_include_status_has_next:
			if (self->current_path)
				*filename = strdup(self->current_path);
			break;

		case mtar_pattern_include_status_init:
			break;
	}
}

struct mtar_pattern_include * mtar_pattern_include_private_new(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_include_private * self = malloc(sizeof(struct mtar_pattern_include_private));
	self->path = strdup(pattern);
	self->option = option;
	self->current_path = NULL;
	self->first = self->last = NULL;
	self->status = mtar_pattern_include_status_init;

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

bool mtar_pattern_match(const struct mtar_option * option, const char * filename) {
	if (option == NULL || filename == NULL)
		return true;

	unsigned int i;
	for (i = 0; i < option->nb_excludes; i++)
		if (option->excludes[i]->ops->match(option->excludes[i], filename))
			return true;

	const char * file = strrchr(filename, '/');
	if (file)
		file++;
	else
		file = filename;

	if (option->exclude_option & mtar_exclude_option_backup) {
		static const char * patterns[] = { ".#*", "*~", "#*#", 0 };

		for (i = 0; patterns[i]; i++)
			if (!fnmatch(patterns[i], file, 0))
				return true;
	}

	if (option->exclude_option & mtar_exclude_option_vcs) {
		static const char * patterns[] = {
			"CVS", "RCS", "SCCS", ".git", ".gitignore", ".cvsignore", ".svn",
			".arch-ids", "{arch}", "=RELEASE-ID", "=meta-update", "=update",
			".bzr", ".bzrignore", ".bzrtags", ".hg", ".hgignore", ".hgrags",
			"_darcs", 0
		};

		for (i = 0; patterns[i]; i++)
			if (!strcmp(patterns[i], file))
				return true;
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
						case mtar_pattern_tag:
						case mtar_pattern_tag_under:
							continue;

						case mtar_pattern_tag_all:
							return true;
					}
				}
			}

			return true;
		}

		for (i = 0; i < option->nb_exclude_tags; i++) {
			if (!strcmp(file, option->exclude_tags[i].tag)) {
				switch (option->exclude_tags[i].option) {
					case mtar_pattern_tag:
						continue;

					case mtar_pattern_tag_all:
					case mtar_pattern_tag_under:
						return true;
				}
			}
		}

		length = file - filename;
		if (length > 0)
			strncpy(path, filename, length);

		for (i = 0; i < option->nb_exclude_tags; i++) {
			strcpy(path + length, option->exclude_tags[i].tag);

			if (strcmp(path, filename) && !access(path, F_OK))
				return true;
		}
	}

	return false;
}

void mtar_pattern_register(struct mtar_pattern_driver * driver) {
	if (driver == NULL || mtar_plugin_check(&driver->api_level))
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

