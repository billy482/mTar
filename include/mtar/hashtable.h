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
*  Last modified: Sat, 20 Oct 2012 00:00:52 +0200                           *
\***************************************************************************/

#ifndef __MTAR_HASHTABLE_H__
#define __MTAR_HASHTABLE_H__

#include <stdbool.h>

typedef unsigned long long (*mtar_hashtable_compupte_hash_f)(const void * key);
typedef void (*mtar_hashtable_free_f)(void * key, void * value);

struct mtar_hashtable {
	struct mtar_hashtable_node {
		unsigned long long hash;
		void * key;
		void * value;
		struct mtar_hashtable_node * next;
	} ** nodes;
	unsigned int nb_elements;
	unsigned int size_node;

	bool allow_rehash;

	mtar_hashtable_compupte_hash_f compupte_hash;
	mtar_hashtable_free_f release_key_value;
};

struct mtar_hashtable * mtar_hashtable_new(mtar_hashtable_compupte_hash_f compupte_hash);
struct mtar_hashtable * mtar_hashtable_new2(mtar_hashtable_compupte_hash_f compupte_hash, mtar_hashtable_free_f release_key_value);
void mtar_hashtable_free(struct mtar_hashtable * hashtable);
bool mtar_hashtable_has_key(struct mtar_hashtable * hashtable, const void * key);
const void ** mtar_hashtable_keys(struct mtar_hashtable * hashtable);
void mtar_hashtable_put(struct mtar_hashtable * hashtable, void * key, void * value);
void * mtar_hashtable_remove(struct mtar_hashtable * hashtable, const void * key);
void * mtar_hashtable_value(struct mtar_hashtable * hashtable, const void * key);
void ** mtar_hashtable_values(struct mtar_hashtable * hashtable);

#endif

