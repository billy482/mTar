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
*  Last modified: Sat, 09 Jun 2012 13:32:23 +0200                           *
\***************************************************************************/

// CU_add_suite, CU_cleanup_registry, CU_cleanup_registry
#include <CUnit/CUnit.h>
// open
#include <fcntl.h>
// printf
#include <stdio.h>
// system
#include <stdlib.h>
// open
#include <sys/stat.h>
// open
#include <sys/types.h>
// _exit
#include <unistd.h>

#include <mtar/format.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/pattern.h>

#include "test.h"

static struct mtar_option option = {
	.filename = "/tmp/test.tar",

	.block_factor = 10,

	.format = "ustar",

	.compress_level = 6,

	.exclude_option = MTAR_EXCLUDE_OPTION_DEFAULT,
	.delimiter      = '\n',
};

static void test_format_ustar_addfile_0(void);
static void test_format_ustar_addfile_1(void);
static void test_format_ustar_addlabel(void);

static struct {
	void (*function)(void);
	char * name;
} test_functions[] = {
	{ test_format_ustar_addfile_0, "mTar: format: ustar addfile #0" },
	{ test_format_ustar_addfile_1, "mTar: format: ustar addfile #1" },

	{ test_format_ustar_addlabel, "mTar: format: ustar label" },

	{ 0, 0 },
};

void test_format_ustar_add_suite() {
	CU_pSuite suite = CU_add_suite("mTar: format: ustar", 0, 0);
	if (!suite) {
		CU_cleanup_registry();
		printf("Error while adding suite mTar format ustar because %s\n", CU_get_error_msg());
		_exit(3);
	}

	int i;
	for (i = 0; test_functions[i].name; i++) {
		if (!CU_add_test(suite, test_functions[i].name, test_functions[i].function)) {
			CU_cleanup_registry();
			printf("Error while adding test function '%s' mTar format ustar because %s\n", test_functions[i].name, CU_get_error_msg());
			_exit(3);
		}
	}
}


void test_format_ustar_addfile_0() {
	struct mtar_format_header header;
	mtar_format_init_header(&header);

	struct mtar_format_out * tar = mtar_format_get_out(&option);

	const char * filename = ".";
	enum mtar_format_out_status status = tar->ops->add_file(tar, filename, &header);
	int last_errno = tar->ops->last_errno(tar);
	tar->ops->free(tar);

	CU_ASSERT_EQUAL(status, MTAR_FORMAT_OUT_OK);
	CU_ASSERT_EQUAL(last_errno, 0);

	struct stat st;
	int failed = stat(filename, &st);
	CU_ASSERT_EQUAL(failed, 0);

	// header
	CU_ASSERT_STRING_EQUAL(header.path, filename);
	CU_ASSERT_STRING_EQUAL(header.link, "");
	CU_ASSERT_EQUAL(header.size, 0);
	CU_ASSERT_EQUAL(header.position, 0);
	CU_ASSERT_EQUAL(header.mode, st.st_mode);
	CU_ASSERT_EQUAL(header.uid, st.st_uid);
	CU_ASSERT_EQUAL(header.gid, st.st_gid);
	CU_ASSERT_EQUAL(header.is_label, 0);
	mtar_format_free_header(&header);

	int tar_status = system("tar tf /tmp/test.tar 2>&1 > /dev/null");
	CU_ASSERT_EQUAL(tar_status, 0);
}

void test_format_ustar_addfile_1() {
	struct mtar_format_header header;
	mtar_format_init_header(&header);

	struct mtar_format_out * tar = mtar_format_get_out(&option);

	const char * filename = "Makefile";
	enum mtar_format_out_status status = tar->ops->add_file(tar, filename, &header);
	CU_ASSERT_EQUAL(status, MTAR_FORMAT_OUT_OK);

	int last_errno = tar->ops->last_errno(tar);
	CU_ASSERT_EQUAL(last_errno, 0);

	int fd = open(filename, O_RDONLY);
	CU_ASSERT_NOT_EQUAL(fd, -1);

	ssize_t nb_read;
	char buffer[4096];
	while ((nb_read = read(fd, buffer, 4096)) > 0) {
		ssize_t nb_write = tar->ops->write(tar, buffer, nb_read);

		CU_ASSERT_EQUAL(nb_write, nb_read);

		last_errno = tar->ops->last_errno(tar);
		CU_ASSERT_EQUAL(last_errno, 0);
	}
	CU_ASSERT_EQUAL(nb_read, 0);

	int failed = tar->ops->end_of_file(tar);
	CU_ASSERT_EQUAL(failed, 0);

	failed = close(fd);
	CU_ASSERT_EQUAL(failed, 0);

	tar->ops->free(tar);

	struct stat st;
	failed = stat(filename, &st);
	CU_ASSERT_EQUAL(failed, 0);

	// header
	CU_ASSERT_STRING_EQUAL(header.path, filename);
	CU_ASSERT_STRING_EQUAL(header.link, "");
	CU_ASSERT_EQUAL(header.size, st.st_size);
	CU_ASSERT_EQUAL(header.position, 0);
	CU_ASSERT_EQUAL(header.mode, st.st_mode);
	CU_ASSERT_EQUAL(header.uid, st.st_uid);
	CU_ASSERT_EQUAL(header.gid, st.st_gid);
	CU_ASSERT_EQUAL(header.is_label, 0);
	mtar_format_free_header(&header);

	failed = system("tar tf /tmp/test.tar > /dev/null");
	CU_ASSERT_EQUAL(failed, 0);
}

void test_format_ustar_addlabel() {
	struct mtar_format_out * tar = mtar_format_get_out(&option);

	const char * label = "foo";
	enum mtar_format_out_status status = tar->ops->add_label(tar, label);
	int last_errno = tar->ops->last_errno(tar);
	tar->ops->free(tar);

	CU_ASSERT_EQUAL(status, MTAR_FORMAT_OUT_OK);
	CU_ASSERT_EQUAL(last_errno, 0);

	int tar_status = system("tar tf /tmp/test.tar > /dev/null");
	CU_ASSERT_EQUAL(tar_status, 0);
}

