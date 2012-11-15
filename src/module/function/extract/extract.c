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
*  Last modified: Thu, 15 Nov 2012 18:52:57 +0100                           *
\***************************************************************************/

// fstatat, futimesat, linkat, openat, symlinkat, unlinkat
#include <fcntl.h>
// bool
#include <stdbool.h>
// free
#include <stdlib.h>
// strerror
#include <string.h>
// fstatat
#include <sys/stat.h>
// futimesat
#include <sys/time.h>
// waitpid
#include <sys/types.h>
// waitpid
#include <sys/wait.h>
// close, execl, _exit, fork, linkat, symlinkat, unlinkat, write
#include <unistd.h>

#include <mtar-function-extract.chcksum>
#include <mtar.version>

#include <mtar/file.h>
#include <mtar/filter.h>
#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

#include "extract.h"

struct mtar_function_extract_param {
	struct mtar_format_reader * format;

	const char * filename;
	unsigned int i_volume;

	const struct mtar_option * option;
};

static int mtar_function_extract(const struct mtar_option * option);
static void mtar_function_extract_init(void) __attribute__((constructor));
static int mtar_function_extract_select_volume(struct mtar_function_extract_param * param);
static void mtar_function_extract_show_description(void);
static void mtar_function_extract_show_help(void);
static void mtar_function_extract_show_version(void);

static struct mtar_function mtar_function_extract_functions = {
	.name             = "extract",

	.do_work           = mtar_function_extract,

	.show_description = mtar_function_extract_show_description,
	.show_help        = mtar_function_extract_show_help,
	.show_version     = mtar_function_extract_show_version,

	.api_level        = {
		.filter   = 0,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = MTAR_FUNCTION_API_LEVEL,
		.io       = MTAR_IO_API_LEVEL,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = MTAR_PATTERN_API_LEVEL,
	},
};


static int mtar_function_extract(const struct mtar_option * option) {
	struct mtar_function_extract_param param = {
		.format = mtar_format_get_reader(option),

		.filename = option->filename,
		.i_volume = 1,

		.option = option,
	};

	if (param.format == NULL)
		return 1;

	mtar_function_extract_configure(option);

	int dir_fd = AT_FDCWD;
	if (option->working_directory != NULL) {
		dir_fd = openat(AT_FDCWD, option->working_directory, O_RDONLY | O_NOCTTY | O_NONBLOCK | O_DIRECTORY | O_CLOEXEC);
		if (dir_fd == -1) {
			mtar_verbose_printf("Fatal error: failed to change directory (%s)\n", option->working_directory);
			return 1;
		}
	}

	struct mtar_format_header header;

	int failed = -1;
	while (failed < 0) {
		enum mtar_format_reader_header_status status = param.format->ops->get_header(param.format, &header);

		bool extract = option->nb_files == 0;
		unsigned int i;
		char * last_slash;

		switch (status) {
			case mtar_format_header_ok:
				for (i = 0; !extract && i < option->nb_files; i++)
					extract = option->files[i]->ops->match(option->files[i], header.path);

				if (!extract || mtar_pattern_match(option, header.path)) {
					status = param.format->ops->skip_file(param.format);
					switch (status) {
						case mtar_format_header_end_of_tape:
							failed = mtar_function_extract_select_volume(&param);
							continue;

						case mtar_format_header_ok:
							break;

						default:
							mtar_verbose_printf("Failed to skip file\n");
							failed = 2;
					}
					continue;
				}

				last_slash = strrchr(header.path, '/');
				if (last_slash != NULL) {
					*last_slash = '\0';

					mtar_file_mkdirat(dir_fd, header.path, header.mode);

					*last_slash = '/';
				}


				mtar_function_extract_display(&header);

				if (header.is_label)
					break;

				if (option->unlink_first || option->recursive_unlink) {
					struct stat st;
					failed = fstatat(dir_fd, header.path, &st, AT_SYMLINK_NOFOLLOW);

					if (failed) {
						failed = -1;
					} else if (option->recursive_unlink && S_ISDIR(st.st_mode)) {
						failed = mtar_file_rmdirat(dir_fd, header.path);
						if (!failed)
							failed = -1;
					} else if (!S_ISDIR(st.st_mode)) {
						failed = unlinkat(dir_fd, header.path, 0);
						if (failed)
							mtar_verbose_printf("Error while unlinking (%s) because %m\n", header.link);
						else
							failed = -1;
					}
				}

				if (header.link != NULL && !(header.mode & S_IFMT)) {
					if (linkat(dir_fd, header.link, dir_fd, header.path, 0))
						mtar_verbose_printf("Error while creating hardlink (from %s to %s) because %m\n", header.link, header.path);
				} else if (S_ISFIFO(header.mode)) {
					if (mknodat(dir_fd, header.path, S_IFIFO, 0))
						mtar_verbose_printf("Error while creating fifo (%s) because %m\n", header.path);
				} else if (S_ISCHR(header.mode)) {
					if (mknodat(dir_fd, header.path, S_IFCHR, header.dev))
						mtar_verbose_printf("Error while creating character device (%s %02x:%02x) because %m\n", header.path, (unsigned int) header.dev >> 8, (unsigned int) header.dev & 0xFF);
				} else if (S_ISDIR(header.mode)) {
					if (mkdirat(dir_fd, header.path, header.mode))
						mtar_verbose_printf("Error while creating directory (%s) because %m\n", header.path);
				} else if (S_ISBLK(header.mode)) {
					if (mknodat(dir_fd, header.path, S_IFBLK, header.dev))
						mtar_verbose_printf("Error while creating block device (%s %02x:%02x) because %m\n", header.path, (unsigned int) header.dev >> 8, (unsigned int) header.dev & 0xFF);
				} else if (S_ISREG(header.mode)) {
					int fd = openat(dir_fd, header.path, O_CREAT | O_TRUNC | O_WRONLY, header.mode);

					if (fd < 0) {
						mtar_verbose_printf("Error while opening file (%s) because %m\n", header.path);
						failed = 3;
						continue;
					} else {
						char buffer[4096];
						ssize_t nb_total_read = 0;
						bool cont = true;

						while (nb_total_read < header.size && cont) {
							ssize_t nb_read = param.format->ops->read(param.format, buffer, 4096);

							if (nb_read > 0) {
								nb_total_read += nb_read;

								ssize_t nb_write = write(fd, buffer, nb_read);
								if (nb_write < 0) {
									mtar_function_extract_clean();
									mtar_verbose_printf("Error while write to (%s) because %m\n", header.path);
								}

								mtar_function_extract_progress(header.path, "\r%b [%P] ETA: %E", nb_total_read, header.size);
							} else if (nb_read == 0) {
								failed = mtar_function_extract_select_volume(&param);
								if (failed != 0)
									break;

								struct mtar_format_header sub_header;
								enum mtar_format_reader_header_status sub_status = param.format->ops->get_header(param.format, &sub_header);

								int error;
								size_t path_length;
								switch (sub_status) {
									case mtar_format_header_ok:
										path_length = strlen(sub_header.path);
										failed = strncmp(header.path, sub_header.path, path_length) || nb_total_read != sub_header.position;

										if (failed) {
											mtar_verbose_printf("Error, it seems you have selected wrong volume archive\n");
											cont = false;
											failed = 4;
										}

										break;

									case mtar_format_header_end_of_tape:
										mtar_verbose_printf("Error, end of tape found while reading header (from %s)\n", param.filename);
										cont = false;
										failed = 4;
										break;

									default:
										error = param.format->ops->last_errno(param.format);
										mtar_verbose_printf("Error while reading header (from %s) because %s\n", param.filename, strerror(error));
										cont = false;
										failed = 5;
										break;
								}
							} else {
								mtar_function_extract_clean();

								int error = param.format->ops->last_errno(param.format);
								mtar_verbose_printf("Error while reading from (%s) because %s\n", param.filename, strerror(error));
							}
						}

						close(fd);
					}
					mtar_function_extract_clean();

				} else if (S_ISLNK(header.mode)) {
					if (symlinkat(header.link, dir_fd, header.path))
						mtar_verbose_printf("Error while creating symlink (from %s to %s) because %m\n", header.link, header.path);
				}

				struct timespec times[2] = {
					{ .tv_sec = header.mtime, .tv_nsec = 0 },
					{ .tv_sec = header.mtime, .tv_nsec = 0 },
				};
				if (utimensat(dir_fd, header.path, times, AT_SYMLINK_NOFOLLOW))
					mtar_verbose_printf("Warning, failed to restore modified time of %s because %m\n", header.path);

				break;

			case mtar_format_header_bad_checksum:
				mtar_verbose_printf("Bad checksum\n");
				failed = 3;
				continue;

			case mtar_format_header_bad_header:
				mtar_verbose_printf("Bad header\n");
				failed = 4;
				continue;

			case mtar_format_header_end_of_tape:
				failed = mtar_function_extract_select_volume(&param);
				continue;

			case mtar_format_header_error:
				mtar_verbose_printf("Error while reader header\n");
				failed = 5;
				continue;

			case mtar_format_header_not_found:
				failed = 0;
				continue;
		}

		mtar_format_free_header(&header);
	}

	close(dir_fd);
	param.format->ops->free(param.format);

	return 0;
}

static void mtar_function_extract_init() {
	mtar_function_register(&mtar_function_extract_functions);
}

static int mtar_function_extract_select_volume(struct mtar_function_extract_param * param) {
	mtar_function_extract_clean();

	for (;;) {
		char * line = mtar_verbose_prompt("Select volume #%u for `%s' and hit return: ", param->i_volume + 1, param->filename);
		if (!line)
			break;

		switch (*line) {
			case 'n': {
				// new filename
					char * filename;
					for (filename = line + 1; *filename == ' '; filename++);

					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(filename, param->option);

					if (next_reader != NULL && param->format != NULL) {
						param->format->ops->next_volume(param->format, next_reader);
						param->i_volume++;
						free(line);
						return 0;
					} else if (next_reader != NULL) {
						param->format = mtar_format_get_reader2(next_reader, param->option);
						param->i_volume++;
						free(line);
						return 0;
					}
				}
				return 0;

			case 'q':
				mtar_verbose_printf("Archive is not fully parsed\n");
				_exit(1);

			case '\0':
			case 'y': {
					// reuse same filename
					struct mtar_io_reader * next_reader = mtar_filter_get_reader3(param->filename, param->option);

					if (next_reader != NULL && param->format != NULL) {
						param->format->ops->next_volume(param->format, next_reader);
						param->i_volume++;
						free(line);
						return 0;
					} else if (next_reader != NULL) {
						param->format = mtar_format_get_reader2(next_reader, param->option);
						param->i_volume++;
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

static void mtar_function_extract_show_description() {
	mtar_verbose_print_help("extract : Extract files from tar archive");
}

static void mtar_function_extract_show_help() {
	mtar_verbose_printf("  List files from tar archive\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
	mtar_verbose_printf("    -H, --format FORMAT : use FORMAT as tar format\n");
	mtar_verbose_printf("    -j, --bzip2         : filter the archive through bzip2\n");
	mtar_verbose_printf("    -z, --gzip          : filter the archive through gzip\n");
	mtar_verbose_printf("    -C, --directory=DIR : change to directory DIR\n");
	mtar_verbose_printf("    -v, --verbose       : verbosely extract files processed\n");
}

static void mtar_function_extract_show_version() {
	mtar_verbose_printf("  extract: Extract files from tar archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("           SHA1 of source files: " MTAR_FUNCTION_EXTRACT_SRCSUM "\n");
}

