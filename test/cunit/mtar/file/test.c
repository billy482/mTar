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
*  Last modified: Sun, 03 Jun 2012 17:50:33 +0200                           *
\***************************************************************************/

// CU_add_suite, CU_cleanup_registry, CU_cleanup_registry
#include <CUnit/CUnit.h>
// printf
#include <stdio.h>
#include <sys/stat.h>
// _exit
#include <unistd.h>

#include <mtar/file.h>

#include "test.h"

static void test_mtar_file_convert_mode(void);

static struct {
	void (*function)(void);
	char * name;
} test_functions[] = {
	{ test_mtar_file_convert_mode, "mtar: file convert mode" },

	{ 0, 0 },
};

void test_mtar_file_add_suite() {
	CU_pSuite suite = CU_add_suite("mTar: file", 0, 0);
	if (!suite) {
		CU_cleanup_registry();
		printf("Error while adding suite mTar file because %s\n", CU_get_error_msg());
		_exit(3);
	}

	int i;
	for (i = 0; test_functions[i].name; i++) {
		if (!CU_add_test(suite, test_functions[i].name, test_functions[i].function)) {
			CU_cleanup_registry();
			printf("Error while adding test function '%s' mTar file because %s\n", test_functions[i].name, CU_get_error_msg());
			_exit(3);
		}
	}
}


void test_mtar_file_convert_mode() {
	char buffer[11];

	mode_t mode = S_IFREG | 0644;
	mtar_file_convert_mode(buffer, mode);
	CU_ASSERT_STRING_EQUAL(buffer, "-rw-r--r--");

	mode = S_IFREG | 0640;
	mtar_file_convert_mode(buffer, mode);
	CU_ASSERT_STRING_EQUAL(buffer, "-rw-r-----");

	mode = S_IFDIR | 0700;
	mtar_file_convert_mode(buffer, mode);
	CU_ASSERT_STRING_EQUAL(buffer, "drwx------");

	mode = S_IFCHR | 01700;
	mtar_file_convert_mode(buffer, mode);
	CU_ASSERT_STRING_EQUAL(buffer, "crwx-----T");
}

