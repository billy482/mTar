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
*  Last modified: Mon, 04 Jun 2012 23:39:09 +0200                           *
\***************************************************************************/

// CU_add_suite, CU_cleanup_registry, CU_cleanup_registry
#include <CUnit/CUnit.h>
// printf
#include <stdio.h>
// free
#include <stdlib.h>
// _exit
#include <unistd.h>

#include <mtar/hashtable.h>
#include <mtar/util.h>

#include "test.h"

static void test_mtar_hashtable_0(void);
static void test_mtar_hashtable_1(void);
static void test_mtar_hashtable_2(void);
static void test_mtar_hashtable_3(void);
static void test_mtar_hashtable_4(void);
static void test_mtar_hashtable_5(void);

static struct {
	void (*function)(void);
	char * name;
} test_functions[] = {
	{ test_mtar_hashtable_0, "mTar: hashtable value #0" },
	{ test_mtar_hashtable_1, "mTar: hashtable value #1" },
	{ test_mtar_hashtable_2, "mTar: hashtable hashKey" },
	{ test_mtar_hashtable_3, "mTar: hashtable keys" },
	{ test_mtar_hashtable_4, "mTar: hashtable put" },
	{ test_mtar_hashtable_5, "mTar: hashtable remove" },

	{ 0, 0 },
};

void test_mtar_hashtable_add_suite() {
	CU_pSuite suite = CU_add_suite("mTar: hashtable", 0, 0);
	if (!suite) {
		CU_cleanup_registry();
		printf("Error while adding suite mTar hashtable because %s\n", CU_get_error_msg());
		_exit(3);
	}

	int i;
	for (i = 0; test_functions[i].name; i++) {
		if (!CU_add_test(suite, test_functions[i].name, test_functions[i].function)) {
			CU_cleanup_registry();
			printf("Error while adding test function '%s' mTar hashtable because %s\n", test_functions[i].name, CU_get_error_msg());
			_exit(3);
		}
	}
}


void test_mtar_hashtable_0() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);

	mtar_hashtable_put(table, "abc", "def");
	mtar_hashtable_put(table, "def", "abc");

	const char * val = mtar_hashtable_value(table, "abc");
	CU_ASSERT_STRING_EQUAL(val, "def");

	val = mtar_hashtable_value(table, "def");
	CU_ASSERT_STRING_EQUAL(val, "abc");

	mtar_hashtable_free(table);
}

void test_mtar_hashtable_1() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);

	mtar_hashtable_put(table, "abc", "def");
	const char * val = mtar_hashtable_value(table, "abc");
	CU_ASSERT_STRING_EQUAL(val, "def");

	mtar_hashtable_put(table, "abc", "ghi");
	val = mtar_hashtable_value(table, "abc");
	CU_ASSERT_STRING_EQUAL(val, "ghi");

	mtar_hashtable_free(table);
}

void test_mtar_hashtable_2() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);

	mtar_hashtable_put(table, "abc", "def");
	short ok = mtar_hashtable_has_key(table, "def");
	CU_ASSERT_EQUAL(ok, 0);

	ok = mtar_hashtable_has_key(table, "abc");
	CU_ASSERT_NOT_EQUAL(ok, 0);

	mtar_hashtable_free(table);
}

void test_mtar_hashtable_3() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);
	mtar_hashtable_put(table, "abc", "def");

	const void ** keys = mtar_hashtable_keys(table);
	CU_ASSERT_PTR_NOT_NULL(keys);

	unsigned int i;
	for (i = 0; keys && keys[i]; i++);
	CU_ASSERT_EQUAL(i, 1);

	free(keys);


	mtar_hashtable_put(table, "abc", "ghi");

	keys = mtar_hashtable_keys(table);
	CU_ASSERT_PTR_NOT_NULL(keys);
	for (i = 0; keys && keys[i]; i++);
	CU_ASSERT_EQUAL(i, 1);

	free(keys);


	mtar_hashtable_put(table, "def", "ghi");

	keys = mtar_hashtable_keys(table);
	CU_ASSERT_PTR_NOT_NULL(keys);
	for (i = 0; keys && keys[i]; i++);
	CU_ASSERT_EQUAL(i, 2);

	free(keys);

	mtar_hashtable_free(table);
}

void test_mtar_hashtable_4() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);
	CU_ASSERT_EQUAL(table->nb_elements, 0);

	mtar_hashtable_put(table, "abc", "def");
	CU_ASSERT_EQUAL(table->nb_elements, 1);

	mtar_hashtable_put(table, "def", "abc");
	CU_ASSERT_EQUAL(table->nb_elements, 2);

	mtar_hashtable_put(table, "abc", "def");
	CU_ASSERT_EQUAL(table->nb_elements, 2);

	mtar_hashtable_free(table);
}

void test_mtar_hashtable_5() {
	struct mtar_hashtable * table = mtar_hashtable_new(mtar_util_compute_hash_string);
	CU_ASSERT_EQUAL(table->nb_elements, 0);

	mtar_hashtable_put(table, "abc", "def");
	CU_ASSERT_EQUAL(table->nb_elements, 1);

	mtar_hashtable_put(table, "def", "abc");
	CU_ASSERT_EQUAL(table->nb_elements, 2);

	mtar_hashtable_put(table, "abc", "def");
	CU_ASSERT_EQUAL(table->nb_elements, 2);

	mtar_hashtable_remove(table, "abc");
	CU_ASSERT_EQUAL(table->nb_elements, 1);

	mtar_hashtable_remove(table, "ghi");
	CU_ASSERT_EQUAL(table->nb_elements, 1);

	mtar_hashtable_free(table);
}

