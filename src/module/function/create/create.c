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
*  Last modified: Tue, 23 Aug 2011 21:30:36 +0200                       *
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
// chdir, read, stat
#include <unistd.h>

#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/hashtable.h>
#include <mtar/option.h>
#include <mtar/plugin.h>
#include <mtar/util.h>
#include <mtar/verbose.h>

#include "common.h"

struct mtar_function_create_param {
	const char * filename;
	struct mtar_format_out * format;
	struct mtar_hashtable * inode;
	const struct mtar_option * option;
};

static int mtar_function_create(const struct mtar_option * option);
static int mtar_function_create2(struct mtar_function_create_param * param);
static int mtar_function_create_filter(const struct dirent * d);
static void mtar_function_create_init(void) __attribute__((constructor));
static void mtar_function_create_show_description(void);
static void mtar_function_create_show_help(void);

static struct mtar_function mtar_function_create_functions = {
	.name             = "create",
	.doWork           = mtar_function_create,
	.show_description = mtar_function_create_show_description,
	.show_help        = mtar_function_create_show_help,
};


int mtar_function_create(const struct mtar_option * option) {
	mtar_function_create_configure(option);

	struct mtar_function_create_param param = {
		.filename = 0,
		.format   = 0,
		.inode    = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free),
		.option   = option,
	};

	param.format = mtar_format_get_out(option);

	if (option->working_directory && chdir(option->working_directory)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Fatal error: failed to change directory (%s)\n", option->working_directory);
		return 1;
	}

	if (option->label) {
		mtar_function_create_display_label(option->label);
		param.format->ops->add_label(param.format, option->label);
	}

	unsigned int i;
	int failed = 0;
	for (i = 0; i < option->nbFiles && !failed; i++) {
		param.filename = option->files[i];
		failed = mtar_function_create2(&param);
	}

	mtar_hashtable_free(param.inode);
	if (failed || !option->verify) {
		param.format->ops->free(param.format);
		return failed;
	}

	struct mtar_format_in * tar_in = param.format->ops->reopenForReading(param.format, option);
	param.format->ops->free(param.format);
	if (!tar_in) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Error: Cannot reopen file for verify\n");
		return 1;
	}

	int ok = -1;
	while (ok < 0) {
		struct mtar_format_header header;
		enum mtar_format_in_header_status status = tar_in->ops->get_header(tar_in, &header);

		struct stat st;
		switch (status) {
			case MTAR_FORMAT_HEADER_OK:
				if (header.is_label)
					break;

				if (lstat(header.path, &st)) {
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: Error while getting information\n", header.path);
					tar_in->ops->skip_file(tar_in);
					continue;
				}

				mtar_function_create_display(header.path, &st, 0);

				if ((st.st_mode & 0777) != (header.mode & 0777))
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: mode differs\n", header.path);

				if (S_ISREG(st.st_mode)) {
					if (st.st_size != header.size)
						mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: size differs\n", header.path);
				} else if (S_ISLNK(st.st_mode)) {
					char link[256];
					readlink(header.path, link, 256);

					if (strcmp(link, header.link))
						mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: link differs\n", header.path);
				} else if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
					if (st.st_rdev != header.dev)
						mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: dev differs\n", header.path);
				}

				if (st.st_mtime != header.mtime)
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: mtime differs => %03o\n", header.path, header.mode);

				if (st.st_uid != header.uid)
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: uid differs => %03o\n", header.path, header.uid);

				if (st.st_gid != header.gid)
					mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "%s: gid differs => %03o\n", header.path, header.gid);


				tar_in->ops->skip_file(tar_in);
				break;

			case MTAR_FORMAT_HEADER_BAD_CHECKSUM:
				mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Bad checksum\n");
				ok = 3;
				continue;

			case MTAR_FORMAT_HEADER_BAD_HEADER:
				mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Bad header\n");
				ok = 4;
				continue;

			case MTAR_FORMAT_HEADER_NOT_FOUND:
				ok = 0;
				continue;
		}
	}

	if (!ok) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Verifying archive ok\n");
	} else {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Verifying archive failed\n");
	}

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
		mtar_function_create_display(param->filename, &st, target);
		int failed = param->format->ops->add_link(param->format, param->filename, target);
		free(key);
		return failed;
	}

	mtar_function_create_display(param->filename, &st, 0);

	mtar_hashtable_put(param->inode, key, strdup(param->filename));

	int failed = param->format->ops->add_file(param->format, param->filename);
	if (failed)
		return failed;

	mtar_plugin_add_file(param->filename);

	if (S_ISREG(st.st_mode)) {
		int fd = open(param->filename, O_RDONLY);

		char * buffer = malloc(1048576);

		ssize_t nbRead;
		ssize_t totalNbRead = 0;
		while ((nbRead = read(fd, buffer, 1048576)) > 0) {
			param->format->ops->write(param->format, buffer, nbRead);

			totalNbRead += nbRead;

			mtar_function_create_progress(param->filename, "\r%b [%P] ETA: %E", totalNbRead, st.st_size);

			mtar_plugin_write(buffer, nbRead);
		}

		param->format->ops->end_of_file(param->format);
		mtar_verbose_clean();

		free(buffer);
		close(fd);

		mtar_plugin_end_of_file();

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

void mtar_function_create_init() {
	mtar_function_register(&mtar_function_create_functions);
}

void mtar_function_create_show_description() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    create : Create new archive\n");
}

void mtar_function_create_show_help() {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "  Create new archive\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -W, --verify        : attempt to verify the archive after writing it\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -V, --label=TEXT    : create archive with volume name TEXT\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -C, --directory=DIR : change to directory DIR\n");
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "    -v, --verbose       : verbosely list files processed\n");
}

