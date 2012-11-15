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
*  Last modified: Thu, 15 Nov 2012 19:44:33 +0100                           *
\***************************************************************************/

// fnmatch
#include <fnmatch.h>
// free, realloc
#include <stdlib.h>
// strcmp, strlen, strrchr
#include <string.h>
// stat
#include <sys/types.h>
// stat
#include <sys/stat.h>
// access, stat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/filter.h>
#include <mtar/option.h>
#include <mtar/readline.h>
#include <mtar/verbose.h>

#include "loader.h"
#include "pattern.h"

static struct mtar_pattern_driver ** mtar_pattern_drivers = NULL;
static unsigned int mtar_pattern_nb_drivers = 0;

static void mtar_pattern_exit(void) __attribute__((destructor));


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

	struct mtar_io_reader * file = mtar_filter_get_reader3(filename, op);
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

	struct mtar_io_reader * file = mtar_filter_get_reader3(filename, op);
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
		return mtar_pattern_default_include_new(pattern, option);

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
	if (driver == NULL || !mtar_plugin_check(&driver->api_level))
		return;

	/**
	 * check if module has been preciously loaded
	 * or another module has the same name
	 */
	unsigned int i;
	for (i = 0; i < mtar_pattern_nb_drivers; i++)
		if (!strcmp(driver->name, mtar_pattern_drivers[i]->name))
			return;

	void * new_addr = realloc(mtar_pattern_drivers, (mtar_pattern_nb_drivers + 1) * sizeof(struct mtar_pattern_driver *));
	if (new_addr == NULL) {
		mtar_verbose_printf("Failed to register '%s'", driver->name);
		return;
	}

	mtar_pattern_drivers = new_addr;
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

