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
*  Last modified: Thu, 15 Nov 2012 11:50:20 +0100                           *
\***************************************************************************/

// errno
#include <errno.h>
// open
#include <fcntl.h>
// free, malloc, realloc
#include <stdlib.h>
// snprintf, sprintf
#include <stdio.h>
// strcmp, strcpy, strdup, strlen
#include <string.h>
// lstat, open
#include <sys/stat.h>
// lstat, open, utime, waitpid
#include <sys/types.h>
// waitpid
#include <sys/wait.h>
// chdir, execl, _exit, fork, lstat, read, readlink
#include <unistd.h>
// utime
#include <utime.h>

#include <mtar-function-create.chcksum>
#include <mtar.version>

#include <mtar/filter.h>
#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/hashtable.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/util.h>
#include <mtar/verbose.h>

#include "create.h"

struct mtar_function_create_param {
	const struct mtar_option * option;

	char * buffer;
	ssize_t block_size;

	struct mtar_format_reader * tar_reader;
	struct mtar_format_writer * tar_writer;

	char ** files;
	unsigned int nb_files;
	unsigned int i_files;
};

static int mtar_function_create(const struct mtar_option * option);
static int mtar_function_create_change_volume(struct mtar_function_create_param * param);
static void mtar_function_create_init(void) __attribute__((constructor));
static int mtar_function_create_select_volume(struct mtar_function_create_param * param);
static void mtar_function_create_show_description(void);
static void mtar_function_create_show_help(void);
static void mtar_function_create_show_version(void);

static struct mtar_function mtar_function_create_functions = {
	.name             = "create",

	.do_work          = mtar_function_create,

	.show_description = mtar_function_create_show_description,
	.show_help        = mtar_function_create_show_help,
	.show_version     = mtar_function_create_show_version,

	.api_level        = {
		.filter   = MTAR_FILTER_API_LEVEL,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = MTAR_FUNCTION_API_LEVEL,
		.io       = MTAR_IO_API_LEVEL,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = MTAR_PATTERN_API_LEVEL,
	},
};


static int mtar_function_create(const struct mtar_option * option) {
	mtar_function_create_configure(option);

	char * filename = NULL;
	struct mtar_hashtable * inode = mtar_hashtable_new2(mtar_util_compute_hash_string, mtar_util_basic_free);

	struct mtar_format_writer * tar_writer = mtar_format_get_writer(option);
	ssize_t block_size = tar_writer->ops->block_size(tar_writer);
	struct mtar_function_create_param param = {
		.option = option,

		.buffer = malloc(block_size),
		.block_size = block_size,

		.tar_reader = 0,
		.tar_writer = tar_writer,

		.files = malloc(sizeof(char *)),
		.nb_files = 1,
		.i_files = 0,
	};

	param.files[0] = option->filename != NULL ? strdup(option->filename) : NULL;

	int failed = 0;
	enum mtar_format_writer_status status;

	if (option->working_directory != NULL && chdir(option->working_directory)) {
		mtar_verbose_printf("Fatal error: failed to change directory (%s)\n", option->working_directory);
		failed = 1;
	}

	if (!failed && option->label != NULL) {
		mtar_function_create_display_label(option->label);
		status = tar_writer->ops->add_label(tar_writer, option->label);

		switch (status) {
			case mtar_format_writer_error:
				failed = 1;
				break;

			case mtar_format_writer_end_of_tape:
				failed = mtar_function_create_change_volume(&param);
				break;

			case mtar_format_writer_ok:
				break;
		}
	}

	unsigned int i;
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
			mtar_format_init_header(&header);

			char key[16];
			snprintf(key, 16, "%x_%lx", (int) st.st_dev, st.st_ino);
			if (mtar_hashtable_has_key(inode, key)) {
				const char * target = mtar_hashtable_value(inode, key);
				if (!strcmp(target, filename))
					continue;

				status = tar_writer->ops->add_link(tar_writer, filename, target, &header);
				mtar_function_create_display(&header, target, 0);

				switch (status) {
					case mtar_format_writer_error:
						failed = 1;
						break;

					case mtar_format_writer_end_of_tape:
						failed = mtar_function_create_change_volume(&param);
						break;

					case mtar_format_writer_ok:
						free(filename);
						mtar_format_free_header(&header);
						continue;
				}

				free(filename);
				mtar_format_free_header(&header);

				if (failed)
					break;
				continue;
			}

			mtar_hashtable_put(inode, strdup(key), filename);

			status = tar_writer->ops->add_file(tar_writer, filename, &header);
			mtar_function_create_display(&header, 0, 0);

			switch (status) {
				case mtar_format_writer_error:
					failed = 1;
					break;

				case mtar_format_writer_end_of_tape:
					failed = mtar_function_create_change_volume(&param);
					break;

				case mtar_format_writer_ok:
					break;
			}

			if (failed)
				break;

			if (S_ISREG(st.st_mode)) {
				int fd = open(filename, O_RDONLY);

				ssize_t nb_read, total_nb_write = 0;
				while (!failed) {
					ssize_t next_read = tar_writer->ops->next_prefered_size(tar_writer);

					nb_read = 0;
					if (next_read > 0)
						nb_read = read(fd, param.buffer, next_read);

					if (nb_read < 0) {
						// error while reading
						failed = 1;
						break;
					}

					if (next_read > 0 && nb_read == 0)
						break;

					ssize_t nb_write = 0;
					if (nb_read > 0)
						nb_write = tar_writer->ops->write(tar_writer, param.buffer, nb_read);

					while (nb_write < 0) {
						int last_errno = tar_writer->ops->last_errno(tar_writer);

						switch (last_errno) {
							case ENOSPC:
								failed = mtar_function_create_change_volume(&param);
								break;

							default:
								failed = 1;
								break;
						}

						if (failed)
							break;

						tar_writer->ops->restart_file(tar_writer, filename, total_nb_write);
						nb_write = tar_writer->ops->write(tar_writer, param.buffer, nb_read);
					}

					while (next_read == 0 || nb_read > nb_write) {
						// end of volume
						failed = mtar_function_create_change_volume(&param);

						if (failed)
							break;

						tar_writer->ops->restart_file(tar_writer, filename, total_nb_write + nb_write);

						if (nb_read > nb_write) {
							ssize_t nb_write2 = tar_writer->ops->write(tar_writer, param.buffer + nb_write, nb_read - nb_write);

							if (nb_write2 > 0)
								nb_write += nb_write2;
						} else
							break;
					}

					if (failed)
						break;

					total_nb_write += nb_write;

					mtar_function_create_progress(filename, "\r[%b @%P] ETA: %E", total_nb_write, st.st_size);

					// mtar_plugin_write(buffer, nb_write);
				}

				if (nb_read < 0) {
					// error while reading
					failed = 1;
				}

				tar_writer->ops->end_of_file(tar_writer);
				mtar_function_create_clean();
				close(fd);
			}

			if (option->atime_preserve == mtar_option_atime_replace) {
				struct utimbuf buf = {
					.actime  = st.st_atime,
					.modtime = st.st_mtime,
				};
				utime(filename, &buf);
			}

			mtar_format_free_header(&header);
		}
	}

	mtar_hashtable_free(inode);

	if (failed || !option->verify) {
		free(param.buffer);

		tar_writer->ops->free(tar_writer);

		for (i = 0; i < param.nb_files; i++)
			free(param.files[i]);
		free(param.files);

		return failed;
	}

	if (param.nb_files == 1) {
		param.tar_reader = tar_writer->ops->reopen_for_reading(tar_writer, option);

		if (!param.tar_reader) {
			mtar_verbose_printf("Error: Cannot reopen file for verify\n");
			return 1;
		}
	} else {
		failed = mtar_function_create_select_volume(&param);
	}
	tar_writer->ops->free(tar_writer);

	int ok = -1;
	while (ok < 0 && param.i_files <= param.nb_files) {
		struct mtar_format_header header;
		enum mtar_format_reader_header_status status = param.tar_reader->ops->get_header(param.tar_reader, &header);

		struct stat st;
		switch (status) {
			case mtar_format_header_bad_checksum:
				mtar_verbose_printf("Bad checksum\n");
				ok = 3;
				continue;

			case mtar_format_header_error:
				mtar_verbose_printf("Error while reading\n");
				ok = 6;
				continue;

			case mtar_format_header_bad_header:
				mtar_verbose_printf("Bad header\n");
				ok = 4;
				continue;

			case mtar_format_header_end_of_tape:
				failed = mtar_function_create_select_volume(&param);
				if (failed)
					ok = 5;
				break;

			case mtar_format_header_ok:
				if (header.is_label || header.position > 0)
					break;

				if (lstat(header.path, &st)) {
					mtar_verbose_printf("%s: Error while getting information\n", header.path);
					param.tar_reader->ops->skip_file(param.tar_reader);
					continue;
				}

				mtar_function_create_display(&header, 0, 1);

				if ((st.st_mode & 0777) != (header.mode & 0777))
					mtar_verbose_printf("%s: mode differs\n", header.path);

				if (S_ISREG(st.st_mode)) {
					if (st.st_size != header.size + header.position && !*header.link)
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

				break;

			case mtar_format_header_not_found:
				ok = 0;
				continue;
		}

		status = param.tar_reader->ops->skip_file(param.tar_reader);

		switch (status) {
			case mtar_format_header_end_of_tape:
				failed = mtar_function_create_select_volume(&param);
				if (failed)
					ok = 5;
				break;

			case mtar_format_header_ok:
				break;

			default:
				mtar_verbose_printf("Error while skipping file\n");
				ok = 6;
				continue;
		}
	}

	if (!ok) {
		mtar_verbose_printf("Verifying archive ok\n");
	} else {
		mtar_verbose_printf("Verifying archive failed\n");
	}

	free(param.buffer);
	param.tar_reader->ops->free(param.tar_reader);

	for (i = 0; i < param.nb_files; i++)
		free(param.files[i]);
	free(param.files);

	return failed;
}

static int mtar_function_create_change_volume(struct mtar_function_create_param * param) {
	mtar_function_create_clean();

	char * last_filename = param->files[param->nb_files - 1];

	for (;;) {
		char * line = mtar_verbose_prompt("Prepare volume #%u for `%s' and hit return: ", param->nb_files, last_filename);
		if (!line)
			break;

		switch (*line) {
			case 'n': {
				// new filename
					char * filename;
					for (filename = line + 1; *filename == ' '; filename++);

					param->files = realloc(param->files, (param->nb_files + 1) * sizeof(char *));
					param->files[param->nb_files] = filename;
					param->nb_files++;

					struct mtar_io_writer * new_file = mtar_filter_get_writer3(filename, param->option);
					param->tar_writer->ops->new_volume(param->tar_writer, new_file);

					ssize_t block_size = param->tar_writer->ops->block_size(param->tar_writer);
					if (block_size != param->block_size) {
						param->block_size = block_size;
						param->buffer = realloc(param->buffer, block_size);
					}
				}
				return 0;

			case 'q':
				mtar_verbose_printf("Archive is not complete\n");
				_exit(1);

			case '\0':
			case 'y': {
					// reuse same filename
					struct mtar_io_writer * new_file = mtar_filter_get_writer3(last_filename, param->option);
					param->tar_writer->ops->new_volume(param->tar_writer, new_file);

					param->files = realloc(param->files, (param->nb_files + 1) * sizeof(char *));
					param->files[param->nb_files] = strdup(last_filename);
					param->nb_files++;

					ssize_t block_size = param->tar_writer->ops->block_size(param->tar_writer);
					if (block_size != param->block_size) {
						param->block_size = block_size;
						param->buffer = realloc(param->buffer, block_size);
					}
				}
				return 0;

			case '!': {
					// spawn shell
					pid_t pid = fork();
					if (pid > 0) {
						int status;
						waitpid(pid, &status, 0);
					} else if (pid == 0) {
						int failed = execl("/bin/bash", "bash", (const char *) NULL);
						_exit(failed);
					} else {
						mtar_verbose_printf("Failed to spawn shell because %m\n");
					}
				}
				break;

			case '?':
				mtar_verbose_printf(" n name        Give a new file name for the next (and subsequent) volume(s)\n");
				mtar_verbose_printf(" q             Abort mtar\n");
				mtar_verbose_printf(" y or newline  Continue operation\n");
				mtar_verbose_printf(" !             Spawn a subshell\n");
				mtar_verbose_printf(" ?             Print this list\n");
				break;
		}

		free(line);
	}

	return 1;
}

static void mtar_function_create_init() {
	mtar_function_register(&mtar_function_create_functions);
}

static int mtar_function_create_select_volume(struct mtar_function_create_param * param) {
	mtar_function_create_clean();

	char * selected_filename = param->files[param->i_files];

	for (;;) {
		char * line = mtar_verbose_prompt("Select volume #%u for `%s' and hit return: ", param->i_files + 1, selected_filename);
		if (!line)
			break;

		switch (*line) {
			case 'n': {
				// new filename
					char * filename;
					for (filename = line + 1; *filename == ' '; filename++);

					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(filename, param->option);

					if (next_reader && param->tar_reader) {
						param->tar_reader->ops->next_volume(param->tar_reader, next_reader);
						param->i_files++;
						free(line);
						return 0;
					} else if (next_reader) {
						param->tar_reader = mtar_format_get_reader2(next_reader, param->option, false);
						param->i_files++;
						free(line);
						return 0;
					}
				}
				return 0;

			case 'q':
				mtar_verbose_printf("Archive is not fully verified\n");
				_exit(1);

			case '\0':
			case 'y': {
					// reuse same filename
					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(selected_filename, param->option);

					if (next_reader && param->tar_reader) {
						param->tar_reader->ops->next_volume(param->tar_reader, next_reader);
						param->i_files++;
						free(line);
						return 0;
					} else if (next_reader) {
						param->tar_reader = mtar_format_get_reader2(next_reader, param->option, false);
						param->i_files++;
						free(line);
						return 0;
					}
				}
				break;

			case '!': {
					// spawn shell
					pid_t pid = fork();
					if (pid > 0) {
						int status;
						waitpid(pid, &status, 0);
					} else if (pid == 0) {
						int failed = execl("/bin/bash", "bash", (const char *) NULL);
						_exit(failed);
					} else {
						mtar_verbose_printf("Failed to spawn shell because %m\n");
					}
				}
				break;

			case '?':
				mtar_verbose_printf(" n name        Give a new file name for the next (and subsequent) volume(s)\n");
				mtar_verbose_printf(" q             Abort mtar\n");
				mtar_verbose_printf(" y or newline  Continue operation\n");
				mtar_verbose_printf(" !             Spawn a subshell\n");
				mtar_verbose_printf(" ?             Print this list\n");
				break;
		}

		free(line);
	}

	return 1;
}

static void mtar_function_create_show_description() {
	mtar_verbose_print_help("create : Create new archive");
}

static void mtar_function_create_show_help() {
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

static void mtar_function_create_show_version() {
	mtar_verbose_printf("  create: create new archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("          SHA1 of source files: " MTAR_FUNCTION_CREATE_SRCSUM "\n");
}

