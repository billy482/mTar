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
*  Last modified: Tue, 18 Oct 2011 18:37:30 +0200                           *
\***************************************************************************/

// versionsort
#define _GNU_SOURCE

// scandir
#include <dirent.h>
// fnmatch
#include <fnmatch.h>
// free, realloc
#include <stdlib.h>
// strcmp, strdup, strlen, strrchr
#include <string.h>
// stat
#include <sys/types.h>
// stat
#include <sys/stat.h>
// access, stat
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

	struct mtar_pattern_include_node {
		struct dirent ** nl;
		struct dirent * nl_next;
		int nb_nl;
		int index_nl;

		struct mtar_pattern_include_node * next;
		struct mtar_pattern_include_node * previous;
	} * nodes, * last;

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
static int mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern);
static void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char * filename, size_t length);
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
	for (ptr = self->nodes; ptr; ptr = ptr->next) {
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

int mtar_pattern_include_private_has_next(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_include_private * self = pattern->data;

	if (self->status == MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED)
		return 0;

	if (self->status == MTAR_PATTERN_INCLUDE_PRIVATE_INIT)
		self->status = MTAR_PATTERN_INCLUDE_PRIVATE_HAS_NEXT;

	return 1;
}

void mtar_pattern_include_private_next(struct mtar_pattern_include * pattern, char * filename, size_t length) {
	struct mtar_pattern_include_private * self = pattern->data;

	strncpy(filename, self->path, length);
	length -= strlen(self->path);

	struct mtar_pattern_include_node * ptr;
	for (ptr = self->nodes; ptr && length > 0; ptr = ptr->next) {
		strcat(filename, "/");
		length--;

		size_t node_length = strlen(ptr->nl_next->d_name);
		if (node_length >= length)
			break;

		strcat(filename, ptr->nl_next->d_name);
		length -= node_length;
	}

	struct stat st;
	struct mtar_pattern_include_node * cnode = self->last;
	if (cnode) {
		if (stat(filename, &st)) {
			self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
			return;
		}

		if (S_ISDIR(st.st_mode)) {
			self->last = cnode->next = malloc(sizeof(struct mtar_pattern_include_private));
			cnode->next->previous = cnode;
			cnode = cnode->next;
			cnode->nl = 0;
			cnode->next = 0;
			cnode->index_nl = 0;

			cnode->nb_nl = scandir(filename, &cnode->nl, mtar_pattern_include_filter, versionsort);
			cnode->nl_next = *cnode->nl;
		} else {
			while (cnode) {
				if (cnode->next) {
					free(cnode->next);
					cnode->next = 0;
				}

				free(cnode->nl_next);
				cnode->nl_next = cnode->nl[++cnode->index_nl];

				if (cnode->index_nl < cnode->nb_nl)
					break;

				free(cnode->nl);

				self->last = cnode = cnode->previous;
			}

			if (!self->last) {
				free(self->nodes);
				self->nodes = 0;
				self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
			}
		}
	} else {
		if (stat(self->path, &st)) {
			self->status = MTAR_PATTERN_INCLUDE_PRIVATE_FINISHED;
			return;
		}

		if (S_ISDIR(st.st_mode)) {
			self->nodes = self->last = cnode = malloc(sizeof(struct mtar_pattern_include_private));
			cnode->nl = 0;
			cnode->next = cnode->previous = 0;
			cnode->index_nl = 0;

			cnode->nb_nl = scandir(self->path, &cnode->nl, mtar_pattern_include_filter, versionsort);
			cnode->nl_next = *cnode->nl;
		}
	}
}

struct mtar_pattern_include * mtar_pattern_include_private_new(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_include_private * self = malloc(sizeof(struct mtar_pattern_include_private));
	self->path = strdup(pattern);
	self->option = option;
	self->nodes = self->last = 0;
	self->status = MTAR_PATTERN_INCLUDE_PRIVATE_INIT;

	struct mtar_pattern_include * pttrn = malloc(sizeof(struct mtar_pattern_include));
	pttrn->ops = &mtar_pattern_include_private_ops;
	pttrn->data = self;

	return pttrn;
}

int mtar_pattern_match(const struct mtar_option * option, const char * filename) {
	if (!option || !filename)
		return 0;

	const char * file = strrchr(filename, '/');
	if (file)
		file++;
	else
		file = filename;

	unsigned int i, ok = 0;
	for (i = 0; i < option->nb_excludes && ok; i++)
		ok = option->excludes[i]->ops->match(option->excludes[i], filename);

	if (!ok)
		return 0;

	if (option->exclude_option & MTAR_EXCLUDE_OPTION_BACKUP) {
		static const char * patterns[] = { ".#*", "*~", "#*#", 0 };

		for (i = 0; patterns[i]; i++)
			if (!fnmatch(patterns[i], file, 0))
				return 0;
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
				return 0;
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
							return 0;
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
						return 0;
				}
			}
		}

		length = file - filename;
		if (length > 0)
			strncpy(path, filename, length);

		for (i = 0; i < option->nb_exclude_tags; i++) {
			strcpy(path + length, option->exclude_tags[i].tag);

			if (strcmp(path, filename) && !access(path, F_OK))
				return 0;
		}
	}

	return 1;
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
	mtar_loader_loadAll("pattern");

	unsigned int i, length = 0;
	for (i = 0; i < mtar_pattern_nb_drivers; i++) {
		unsigned int ll = strlen(mtar_pattern_drivers[i]->name);
		if (ll > length)
			length = ll;
	}

	for (i = 0; i < mtar_pattern_nb_drivers; i++) {
		mtar_verbose_printf("    %-*s : ", length, mtar_pattern_drivers[i]->name);
		mtar_pattern_drivers[i]->show_description();
	}
}

