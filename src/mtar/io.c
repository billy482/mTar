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
*  Last modified: Thu, 15 Nov 2012 11:16:40 +0100                           *
\***************************************************************************/

// errno
#include <errno.h>
// open
#include <fcntl.h>
// free, realloc
#include <stdlib.h>
// strerror, strcmp
#include <string.h>
// fstat, open
#include <sys/stat.h>
// fstat, open
#include <sys/types.h>
// access, fstat
#include <unistd.h>

#include <mtar/option.h>
#include <mtar/verbose.h>

#include "io.h"
#include "loader.h"

static struct mtar_io ** mtar_io_ios = NULL;
static unsigned int mtar_io_nbIos = 0;

static void mtar_io_exit(void) __attribute__((destructor));
struct mtar_io * mtar_io_get(int fd);
int mtar_io_open(const char * filename, int flags);


bool mtar_io_check_standard_input_output() {
	struct stat st;

	int i;
	for (i = 0; i < 3; i++)
		if (fstat(i, &st))
			return false;

	return true;
}

void mtar_io_exit() {
	free(mtar_io_ios);
	mtar_io_ios = NULL;
}

struct mtar_io * mtar_io_get(int fd) {
	struct stat st;

	if (fstat(fd, &st)) {
		mtar_verbose_printf("Getting information about file descriptor (%d) failed => %s\n", fd, strerror(errno));
		return NULL;
	}

	const char * module = "pipe";
	if (S_ISREG(st.st_mode))
		module = "file";
	else if (S_ISCHR(st.st_mode) && (st.st_rdev >> 8) == 9)
		module = "tape";

	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		if (!strcmp(module, mtar_io_ios[i]->name))
			return mtar_io_ios[i];

	if (mtar_loader_load("io", module))
		return NULL;

	for (i = 0; i < mtar_io_nbIos; i++)
		if (!strcmp(module, mtar_io_ios[i]->name))
			return mtar_io_ios[i];

	return NULL;
}

struct mtar_io_reader * mtar_io_reader_get_fd(int fd, const struct mtar_option * option, const struct mtar_hashtable * params) {
	struct mtar_io * io = mtar_io_get(fd);

	if (io != NULL && io->new_reader != NULL)
		return io->new_reader(fd, option, params);

	return NULL;
}

struct mtar_io_reader * mtar_io_reader_get_file(const char * filename, int flags, const struct mtar_option * option, const struct mtar_hashtable * params) {
	int fd = mtar_io_open(filename, flags);
	if (fd < 0)
		return NULL;

	return mtar_io_reader_get_fd(fd, option, params);
}

int mtar_io_open(const char * filename, int flags) {
	if (!strcmp("-", filename)) {
		if (!(flags & (O_WRONLY | O_RDWR)))
			return 0;
		if ((flags & O_WRONLY) || (flags & O_RDWR))
			return 1;
	}

	int m = F_OK;
	if (flags & O_RDWR)
		m |= R_OK | W_OK;
	else if (flags & O_RDONLY)
		m |= R_OK;
	else if (flags & O_WRONLY)
		m |= W_OK;

	if (access(filename, m)) {
		if (flags & O_RDONLY) {
			mtar_verbose_printf("Can't open file (%s) for reading if file did not exist\n", filename);
			return -1;
		}
		flags |= O_CREAT;
	}

	int fd = open(filename, flags, 0644);
	if (fd < 0)
		mtar_verbose_printf("Failed to open (%s) => %s\n", filename, strerror(errno));

	return fd;
}

struct mtar_io_writer * mtar_io_writer_get_fd(int fd, const struct mtar_option * option, const struct mtar_hashtable * params) {
	struct mtar_io * io = mtar_io_get(fd);

	if (io != NULL && io->new_writer != NULL)
		return io->new_writer(fd, option, params);

	return NULL;
}

struct mtar_io_writer * mtar_io_writer_get_file(const char * filename, int flags, const struct mtar_option * option, const struct mtar_hashtable * params) {
	int fd = mtar_io_open(filename, flags);
	if (fd < 0)
		return NULL;

	return mtar_io_writer_get_fd(fd, option, params);
}

void mtar_io_register(struct mtar_io * io) {
	if (io == NULL || !mtar_plugin_check(&io->api_level))
		return;

	/**
	 * check if module has been preciously loaded
	 * or another module has the same name
	 */
	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		if (!strcmp(io->name, mtar_io_ios[i]->name))
			return;

	void * new_addr = realloc(mtar_io_ios, (mtar_io_nbIos + 1) * sizeof(struct mtar_io *));
	if (new_addr == NULL) {
		mtar_verbose_printf("Failed to register '%s', not enough memory", io->name);
		return;
	}

	mtar_io_ios = new_addr;
	mtar_io_ios[mtar_io_nbIos] = io;
	mtar_io_nbIos++;

	mtar_loader_register_ok();
}

void mtar_io_show_description() {
	mtar_loader_load_all("io");
	mtar_verbose_printf("List of available back end ios :\n");

	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		mtar_io_ios[i]->show_description();
	mtar_verbose_print_flush(2, 0);
}

void mtar_io_show_version() {
	mtar_loader_load_all("io");
	mtar_verbose_printf("List of available backend ios :\n");

	unsigned int i;
	for (i = 0; i < mtar_io_nbIos; i++)
		mtar_io_ios[i]->show_version();

	mtar_verbose_printf("\n");
}

