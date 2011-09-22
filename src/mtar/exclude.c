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
*  Last modified: Thu, 22 Sep 2011 18:49:02 +0200                           *
\***************************************************************************/

// fnmatch
#include <fnmatch.h>
// free, realloc
#include <stdlib.h>
// strcat, strcmp, strcpy, strlen, strrchr
#include <string.h>
// stat
#include <sys/types.h>
// stat
#include <sys/stat.h>
// access, stat
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "exclude.h"
#include "loader.h"

static struct mtar_exclude_driver ** mtar_exclude_exs = 0;
static unsigned int mtar_exclude_nbExs = 0;

static void mtar_exclude_exit(void) __attribute__((destructor));


struct mtar_exclude_tag * mtar_exclude_add_tag(struct mtar_exclude_tag * tags, unsigned int * nb_tags, const char * tag, enum mtar_exclude_tag_option option) {
	tags = realloc(tags, (*nb_tags + 1) * sizeof(struct mtar_exclude_tag));
	tags[*nb_tags].tag = tag;
	tags[*nb_tags].option = option;
	(*nb_tags)++;
	return tags;
}

int mtar_exclude_filter(struct mtar_exclude * ex, const char * filename, const struct mtar_option * option) {
	if (!option)
		return 0;

	const char * file = strrchr(filename, '/');
	if (file)
		file++;
	else
		file = filename;

	if (ex && ex->ops->filter(ex, filename))
		return 0;

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
						case MTAR_EXCLUDE_TAG:
						case MTAR_EXCLUDE_TAG_UNDER:
							continue;

						case MTAR_EXCLUDE_TAG_ALL:
							return 1;
					}
				}
			}

			return 0;
		}

		for (i = 0; i < option->nb_exclude_tags; i++) {
			if (!strcmp(file, option->exclude_tags[i].tag)) {
				switch (option->exclude_tags[i].option) {
					case MTAR_EXCLUDE_TAG:
						continue;

					case MTAR_EXCLUDE_TAG_ALL:
					case MTAR_EXCLUDE_TAG_UNDER:
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

void mtar_exclude_exit() {
	if (mtar_exclude_nbExs > 0)
		free(mtar_exclude_exs);
	mtar_exclude_exs = 0;
}

struct mtar_exclude * mtar_exclude_get(const struct mtar_option * option) {
	if (option->nb_excludes < 1)
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
	if (!driver || driver->api_version != MTAR_EXCLUDE_API_VERSION)
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
		mtar_verbose_printf("    %-*s : ", length, mtar_exclude_exs[i]->name);
		mtar_exclude_exs[i]->show_description();
	}
}

