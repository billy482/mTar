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
*  Last modified: Fri, 16 Nov 2012 11:20:02 +0100                           *
\***************************************************************************/

// versionsort
#define _GNU_SOURCE

// scandir
#include <dirent.h>
// openat
#include <fcntl.h>
// asprintf
#include <stdio.h>
// free, malloc
#include <stdlib.h>
// memmove, strdup, strlen, strncmp, strstr
#include <string.h>
// lstat
#include <sys/types.h>
// lstat
#include <sys/stat.h>
// close, lstat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/option.h>

#include "pattern.h"

struct mtar_pattern_default_include {
	int dir_fd;
	char * root_directory;

	char * path;
	size_t spath;
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

static void mtar_pattern_default_include_free(struct mtar_pattern_include * pattern);
static bool mtar_pattern_default_include_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option);
static bool mtar_pattern_default_include_match(struct mtar_pattern_include * pattern, const char * filename);
static void mtar_pattern_default_include_next(struct mtar_pattern_include * pattern, char ** filename);

static struct mtar_pattern_include_ops mtar_pattern_default_include_ops = {
	.free     = mtar_pattern_default_include_free,
	.has_next = mtar_pattern_default_include_has_next,
	.match    = mtar_pattern_default_include_match,
	.next     = mtar_pattern_default_include_next,
};


void mtar_pattern_default_include_free(struct mtar_pattern_include * pattern) {
	struct mtar_pattern_default_include * self = pattern->data;

	if (self->dir_fd != AT_FDCWD)
		close(self->dir_fd);
	self->dir_fd = AT_FDCWD;

	free(self->root_directory);
	self->root_directory = NULL;

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

bool mtar_pattern_default_include_has_next(struct mtar_pattern_include * pattern, const struct mtar_option * option) {
	struct mtar_pattern_default_include * self = pattern->data;

	free(self->current_path);
	self->current_path = NULL;

	size_t length;
	struct mtar_pattern_include_node * ptr, * cnode = self->last;
	struct stat st;
	char * path;

	switch (self->status) {
		case mtar_pattern_include_status_finished:
			return false;

		case mtar_pattern_include_status_has_next:
			if (cnode == NULL && self->option & mtar_pattern_option_recursion) {
				cnode = malloc(sizeof(struct mtar_pattern_default_include));

				cnode->nl = NULL;
				cnode->nl_next = NULL;
				cnode->next = cnode->previous = NULL;
				cnode->index_nl = 0;
				cnode->next = cnode->previous = NULL;

				if (self->root_directory != NULL)
					asprintf(&path, "%s/%s", self->root_directory, self->path);
				else
					path = self->path;

				cnode->nb_nl = scandir(path, &cnode->nl, mtar_file_basic_filter, versionsort);

				if (self->root_directory != NULL)
					free(path);

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

				if (fstatat(self->dir_fd, self->current_path, &st, AT_SYMLINK_NOFOLLOW)) {
					free(self->current_path);
					self->current_path = 0;

					self->status = mtar_pattern_include_status_finished;
					return false;
				}

				if (mtar_pattern_match(option, self->current_path)) {
					free(self->current_path);
					self->current_path = NULL;
				} else if (S_ISDIR(st.st_mode)) {
					struct mtar_pattern_include_node * csubnode = malloc(sizeof(struct mtar_pattern_default_include));
					csubnode->next = csubnode->previous = NULL;

					csubnode->nl = NULL;
					csubnode->nl_next = NULL;
					csubnode->index_nl = 0;

					if (self->root_directory != NULL)
						asprintf(&path, "%s/%s", self->root_directory, self->current_path);
					else
						path = self->current_path;

					csubnode->nb_nl = scandir(path, &csubnode->nl, mtar_file_basic_filter, versionsort);

					if (self->root_directory != NULL)
						free(path);

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
			if (fstatat(self->dir_fd, self->path, &st, AT_SYMLINK_NOFOLLOW)) {
				self->status = mtar_pattern_include_status_finished;
				if (self->root_directory != NULL)
					free(path);
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

static bool mtar_pattern_default_include_match(struct mtar_pattern_include * pattern, const char * filename) {
	struct mtar_pattern_default_include * self = pattern->data;
	return !strncmp(filename, self->path, self->spath);
}

void mtar_pattern_default_include_next(struct mtar_pattern_include * pattern, char ** filename) {
	struct mtar_pattern_default_include * self = pattern->data;

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

struct mtar_pattern_include * mtar_pattern_default_include_new(const char * root_directory, const char * pattern, enum mtar_pattern_option option) {
	int dir_fd = AT_FDCWD;
	if (root_directory != NULL) {
		dir_fd = openat(AT_FDCWD, root_directory, O_RDONLY | O_NOCTTY | O_NONBLOCK | O_DIRECTORY | O_CLOEXEC);
		if (dir_fd == -1)
			return NULL;
	}

	struct mtar_pattern_default_include * self = malloc(sizeof(struct mtar_pattern_default_include));

	self->dir_fd = dir_fd;
	self->root_directory = NULL;
	if (root_directory != NULL)
		self->root_directory = strdup(root_directory);

	self->path = strdup(pattern);
	self->spath = 0;
	self->option = option;
	self->current_path = NULL;
	self->first = self->last = NULL;
	self->status = mtar_pattern_include_status_init;

	// remove double '/'
	char * ptr = self->path;
	while ((ptr = strstr(ptr, "//")))
		memmove(ptr, ptr + 1, strlen(ptr));

	if (self->root_directory != NULL) {
		ptr = self->root_directory;
		while ((ptr = strstr(ptr, "//")))
			memmove(ptr, ptr + 1, strlen(ptr));
	}

	// remove trailing '/'
	size_t length = strlen(self->path);
	while (length > 0 && self->path[length - 1] == '/') {
		length--;
		self->path[length] = '\0';
	}

	if (self->root_directory != NULL) {
		length = strlen(self->root_directory);
		while (length > 0 && self->root_directory[length - 1] == '/') {
			length--;
			self->root_directory[length] = '\0';
		}
	}

	self->spath = strlen(self->path);

	struct mtar_pattern_include * pttrn = malloc(sizeof(struct mtar_pattern_include));
	pttrn->ops = &mtar_pattern_default_include_ops;
	pttrn->data = self;

	return pttrn;
}

