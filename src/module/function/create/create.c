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
*  Last modified: Thu, 28 Apr 2011 10:39:11 +0200                       *
\***********************************************************************/

#define _GNU_SOURCE
// scandir
#include <dirent.h>
// open
#include <fcntl.h>
// malloc
#include <stdlib.h>
// snprintf, sprintf
#include <stdio.h>
// strcmp, strcpy, strdup, strlen
#include <string.h>
// open, stat
#include <sys/stat.h>
// open, stat
#include <sys/types.h>
// read, stat
#include <unistd.h>

#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/hashtable.h>
#include <mtar/option.h>
#include <mtar/util.h>
#include <mtar/verbose.h>

struct mtar_function_create_param {
	const char * filename;
	struct mtar_format * format;
	struct mtar_io * io;
	struct mtar_hashtable * inode;
	struct mtar_option * option;
};

static int mtar_function_create(struct mtar_io * io, struct mtar_option * option);
static int mtar_function_create2(struct mtar_function_create_param * param);
static int mtar_function_create_filter(const struct dirent * d);

__attribute__((constructor))
static void mtar_function_create_init() {
	mtar_function_register("create", mtar_function_create);
}


int mtar_function_create(struct mtar_io * io, struct mtar_option * option) {
	struct mtar_function_create_param param = {
		.filename = 0,
		.format   = mtar_format_get(io, option),
		.io       = io,
		.inode    = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free),
		.option   = option,
	};

	unsigned int i;
	int failed = 0;
	for (i = 0; i < option->nbFiles && !failed; i++) {
		param.filename = option->files[i];
		failed = mtar_function_create2(&param);
	}

	mtar_hashtable_free(param.inode);
	return failed;
}

int mtar_function_create2(struct mtar_function_create_param * param) {
	struct stat st;
	if (lstat(param->filename, &st))
		return 1;

	if (S_ISSOCK(st.st_mode))
		return 0;

	char * key = malloc(16);
	snprintf(key, 16, "%llx_%lx", (long long int) st.st_dev, st.st_ino);
	if (mtar_hashtable_hasKey(param->inode, key)) {
		const char * target = mtar_hashtable_value(param->inode, key);
		int failed = param->format->ops->addLink(param->format, param->filename, target);
		free(key);
		return failed;
	}

	mtar_hashtable_put(param->inode, key, strdup(param->filename));

	int failed = param->format->ops->addFile(param->format, param->filename);
	if (failed)
		return failed;

	if (S_ISREG(st.st_mode)) {
		int fd = open(param->filename, O_RDONLY);

		char * buffer = malloc(1048576);

		ssize_t nbRead;
		while ((nbRead = read(fd, buffer, 1048576)) > 0) {
			param->format->ops->write(param->format, buffer, nbRead);
		}

		param->format->ops->endOfFile(param->format);

		free(buffer);

	} else if (S_ISDIR(st.st_mode)) {
		const char * dirname = param->filename;

		struct dirent ** namelist = 0;
		int i, nbFiles = scandir(dirname, &namelist, mtar_function_create_filter, versionsort);
		for (i = 0; i < nbFiles; i++) {
			size_t dirlength = strlen(dirname);
			char * subfile = malloc(dirlength + strlen(namelist[i]->d_name) + 2);

			strcpy(subfile, dirname);
			for (dirlength--; dirlength > 0 && subfile[dirlength] == '/'; dirlength--)
				subfile[dirlength] = '\0';
			sprintf(subfile + dirlength + 1, "/%s", namelist[i]->d_name);
			free(namelist[i]);

			param->filename = subfile;
			failed = mtar_function_create2(param);

			free(subfile);
		}
		free(namelist);

		param->filename = dirname;
	}

	return 0;
}

int mtar_function_create_filter(const struct dirent * d) {
	return !strcmp(".", d->d_name) || !strcmp("..", d->d_name) ? 0 : 1;
}

