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
*  Last modified: Thu, 17 May 2012 23:37:07 +0200                           *
\***************************************************************************/

// open
#include <fcntl.h>
// free, malloc
#include <stdlib.h>
// snprintf, sprintf
#include <stdio.h>
// strcmp, strcpy, strdup, strlen
#include <string.h>
// lstat, open
#include <sys/stat.h>
// lstat, open, utime
#include <sys/types.h>
// chdir, lstat, read, readlink
#include <unistd.h>
// utime
#include <utime.h>

#include <mtar-function-create.chcksum>
#include <mtar.version>

#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/hashtable.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/util.h>
#include <mtar/verbose.h>

#include "create.h"

struct mtar_function_create_param {
	struct mtar_format_out * format;
	char * buffer;
	ssize_t block_size;
	struct mtar_hashtable * inode;
	const struct mtar_option * option;
};

static int mtar_function_create(const struct mtar_option * option);
static void mtar_function_create_init(void) __attribute__((constructor));
static void mtar_function_create_show_description(void);
static void mtar_function_create_show_help(void);
static void mtar_function_create_show_version(void);

static struct mtar_function mtar_function_create_functions = {
	.name             = "create",

	.do_work          = mtar_function_create,

	.show_description = mtar_function_create_show_description,
	.show_help        = mtar_function_create_show_help,
	.show_version     = mtar_function_create_show_version,

	.api_version      = MTAR_FUNCTION_API_VERSION,
};


int mtar_function_create(const struct mtar_option * option) {
	mtar_function_create_configure(option);

	char * filename = 0;
	struct mtar_format_out * format = mtar_format_get_out(option);
	ssize_t block_size = format->ops->block_size(format);
	char * buffer = malloc(block_size);
	struct mtar_hashtable * inode = mtar_hashtable_new2(mtar_util_compute_hash_string, mtar_util_basic_free);

	if (option->working_directory && chdir(option->working_directory)) {
		mtar_verbose_printf("Fatal error: failed to change directory (%s)\n", option->working_directory);
		return 1;
	}

	if (option->label) {
		mtar_function_create_display_label(option->label);
		format->ops->add_label(format, option->label);
	}

	unsigned int i;
	int failed = 0;
	for (i = 0; i < option->nb_files && !failed; i++) {
		while (option->files[i]->ops->has_next(option->files[i], option)) {
			option->files[i]->ops->next(option->files[i], &filename);

			struct stat st;
			if (lstat(filename, &st)) {
				failed = 1;
				break;
			}

			if (S_ISSOCK(st.st_mode))
				continue;

			struct mtar_format_header header;

			char key[16];
			snprintf(key, 16, "%x_%lx", (int) st.st_dev, st.st_ino);
			if (mtar_hashtable_hasKey(inode, key)) {
				const char * target = mtar_hashtable_value(inode, key);
				if (!strcmp(target, filename))
					continue;
				failed = format->ops->add_link(format, filename, target, &header);
				mtar_function_create_display(&header, target, 0);
				if (failed)
					break;
				continue;
			}

			mtar_hashtable_put(inode, strdup(key), strdup(filename));

			failed = format->ops->add_file(format, filename, &header);
			mtar_function_create_display(&header, 0, 0);

			if (failed)
				break;

			if (S_ISREG(st.st_mode)) {
				int fd = open(filename, O_RDONLY);

				ssize_t nbRead;
				ssize_t totalNbRead = 0;
				while ((nbRead = read(fd, buffer, block_size)) > 0) {
					format->ops->write(format, buffer, nbRead);

					totalNbRead += nbRead;

					mtar_function_create_progress(filename, "\r%b [%P] ETA: %E", totalNbRead, st.st_size);

					// mtar_plugin_write(buffer, nbRead);
				}

				format->ops->end_of_file(format);
				mtar_verbose_clean();
				close(fd);
			}

			if (option->atime_preserve == MTAR_OPTION_ATIME_REPLACE) {
				struct utimbuf buf = {
					.actime  = st.st_atime,
					.modtime = st.st_mtime,
				};
				utime(filename, &buf);
			}

			free(filename);
		}
	}

	free(buffer);
	mtar_hashtable_free(inode);

	if (failed || !option->verify) {
		format->ops->free(format);
		return failed;
	}

	struct mtar_format_in * tar_in = format->ops->reopen_for_reading(format, option);
	format->ops->free(format);
	if (!tar_in) {
		mtar_verbose_printf("Error: Cannot reopen file for verify\n");
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
					mtar_verbose_printf("%s: Error while getting information\n", header.path);
					tar_in->ops->skip_file(tar_in);
					continue;
				}

				mtar_function_create_display(&header, 0, 1);

				if ((st.st_mode & 0777) != (header.mode & 0777))
					mtar_verbose_printf("%s: mode differs\n", header.path);

				if (S_ISREG(st.st_mode)) {
					if (st.st_size != header.size && !*header.link)
						mtar_verbose_printf("%s: size differs\n", header.path);
				} else if (S_ISLNK(st.st_mode)) {
					char link[256];
					ssize_t slink = readlink(header.path, link, 256);
					if (slink >= 0) {
						link[slink] = '\0';

						if (strcmp(link, header.link))
							mtar_verbose_printf("%s: link differs\n", header.path);
					}
				} else if (S_ISBLK(st.st_mode) || S_ISCHR(st.st_mode)) {
					if (st.st_rdev != header.dev)
						mtar_verbose_printf("%s: dev differs\n", header.path);
				}

				if (st.st_mtime != header.mtime)
					mtar_verbose_printf("%s: mtime differs => %03o\n", header.path, header.mode);

				if (st.st_uid != header.uid)
					mtar_verbose_printf("%s: uid differs => %03o\n", header.path, header.uid);

				if (st.st_gid != header.gid)
					mtar_verbose_printf("%s: gid differs => %03o\n", header.path, header.gid);


				tar_in->ops->skip_file(tar_in);
				break;

			case MTAR_FORMAT_HEADER_BAD_CHECKSUM:
				mtar_verbose_printf("Bad checksum\n");
				ok = 3;
				continue;

			case MTAR_FORMAT_HEADER_BAD_HEADER:
				mtar_verbose_printf("Bad header\n");
				ok = 4;
				continue;

			case MTAR_FORMAT_HEADER_NOT_FOUND:
				ok = 0;
				continue;
		}
	}

	if (!ok) {
		mtar_verbose_printf("Verifying archive ok\n");
	} else {
		mtar_verbose_printf("Verifying archive failed\n");
	}

	return failed;
}

void mtar_function_create_init() {
	mtar_function_register(&mtar_function_create_functions);
}

void mtar_function_create_show_description() {
	mtar_verbose_print_help("create : Create new archive");
}

void mtar_function_create_show_help() {
	mtar_verbose_printf("  Create new archive\n");
	mtar_verbose_print_help("-W, --verify : attempt to verify the archive after writing it");
	mtar_verbose_print_help("--atime-preserve : preserve access times on dumped files");
	mtar_verbose_print_help("--group=NAME : force NAME as group for added files");
	mtar_verbose_print_help("--mode=CHANGES : force (symbolic) mode CHANGES for added files");
	mtar_verbose_print_help("--owner=NAME : force NAME as owner for added files");
	mtar_verbose_print_help("-f, --file=ARCHIVE : use ARCHIVE file or device ARCHIVE");
	mtar_verbose_print_help("-H, --format FORMAT : use FORMAT as tar format");
	mtar_verbose_print_help("-V, --label=TEXT : create archive with volume name TEXT");
	mtar_verbose_print_help("-j, --bzip2 : filter the archive through bzip2");
	mtar_verbose_print_help("-z, --gzip : filter the archive through gzip");
	mtar_verbose_print_help("-C, --directory=DIR : change to directory DIR before creating archive");
	mtar_verbose_print_help("-v, --verbose : verbosely list files processed");
	mtar_verbose_print_flush(4, 0);
}

void mtar_function_create_show_version() {
	mtar_verbose_printf("  create: create new archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("          SHA1 of source files: " MTAR_FUNCTION_CREATE_SRCSUM "\n");
}

