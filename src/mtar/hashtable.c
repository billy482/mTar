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
*  Last modified: Sat, 20 Oct 2012 00:03:36 +0200                           *
\***************************************************************************/

// calloc, free, malloc
#include <stdlib.h>

#include <mtar/hashtable.h>

void mtar_hashtable_put2(struct mtar_hashtable * hashtable, unsigned int index, struct mtar_hashtable_node * new_node);
void mtar_hashtable_rehash(struct mtar_hashtable * hashtable);


void mtar_hashtable_free(struct mtar_hashtable * hashtable) {
	if (hashtable == NULL)
		return;

	unsigned int i;
	for (i = 0; i < hashtable->size_node; i++) {
		struct mtar_hashtable_node * ptr = hashtable->nodes[i];
		while (ptr != NULL) {
			struct mtar_hashtable_node * tmp = ptr;
			ptr = ptr->next;
			if (hashtable->release_key_value)
				hashtable->release_key_value(tmp->key, tmp->value);
			tmp->key = tmp->value = tmp->next = 0;
			free(tmp);
		}
	}
	free(hashtable->nodes);
	hashtable->nodes = 0;
	hashtable->compupte_hash = 0;

	free(hashtable);
}

bool mtar_hashtable_has_key(struct mtar_hashtable * hashtable, const void * key) {
	if (hashtable == NULL || key == NULL)
		return false;

	unsigned long long hash = hashtable->compupte_hash(key);
	unsigned int index = hash % hashtable->size_node;

	const struct mtar_hashtable_node * node = hashtable->nodes[index];
	while (node != NULL) {
		if (node->hash == hash)
			return true;

		node = node->next;
	}

	return false;
}

const void ** mtar_hashtable_keys(struct mtar_hashtable * hashtable) {
	if (hashtable == NULL)
		return NULL;

	const void ** keys = calloc(sizeof(void *), hashtable->nb_elements + 1);
	unsigned int iNode = 0, index = 0;
	while (iNode < hashtable->size_node) {
		struct mtar_hashtable_node * node = hashtable->nodes[iNode];
		while (node != NULL) {
			keys[index] = node->key;
			index++;
			node = node->next;
		}
		iNode++;
	}
	keys[index] = 0;

	return keys;
}

struct mtar_hashtable * mtar_hashtable_new(mtar_hashtable_compupte_hash_f compupte_hash) {
	return mtar_hashtable_new2(compupte_hash, 0);
}

struct mtar_hashtable * mtar_hashtable_new2(mtar_hashtable_compupte_hash_f compupte_hash, mtar_hashtable_free_f release_key_value) {
	if (compupte_hash == NULL)
		return NULL;

	struct mtar_hashtable * l_hash = malloc(sizeof(struct mtar_hashtable));

	l_hash->nodes = calloc(sizeof(struct mtar_hashtable_node *), 16);
	l_hash->nb_elements = 0;
	l_hash->size_node = 16;
	l_hash->allow_rehash = true;
	l_hash->compupte_hash = compupte_hash;
	l_hash->release_key_value = release_key_value;

	return l_hash;
}

void mtar_hashtable_put(struct mtar_hashtable * hashtable, void * key, void * value) {
	if (hashtable == NULL || key == NULL || value == NULL)
		return;

	unsigned long long hash = hashtable->compupte_hash(key);
	unsigned int index = hash % hashtable->size_node;

	struct mtar_hashtable_node * new_node = malloc(sizeof(struct mtar_hashtable_node));
	new_node->hash = hash;
	new_node->key = key;
	new_node->value = value;
	new_node->next = NULL;

	mtar_hashtable_put2(hashtable, index, new_node);
	hashtable->nb_elements++;
}

void mtar_hashtable_put2(struct mtar_hashtable * hashtable, unsigned int index, struct mtar_hashtable_node * new_node) {
	struct mtar_hashtable_node * node = hashtable->nodes[index];
	if (node != NULL) {
		if (node->hash == new_node->hash) {
			if (hashtable->release_key_value && node->key != new_node->key && node->value != new_node->value)
				hashtable->release_key_value(node->key, node->value);

			hashtable->nodes[index] = new_node;
			new_node->next = node->next;
			free(node);
			hashtable->nb_elements--;
		} else {
			short nbElement = 1;
			while (node->next != NULL) {
				if (node->next->hash == new_node->hash) {
					struct mtar_hashtable_node * next = node->next;

					if (hashtable->release_key_value && next->key != new_node->key && next->value != new_node->value)
						hashtable->release_key_value(next->key, next->value);

					new_node->next = next->next;
					node->next = new_node;
					free(next);
					hashtable->nb_elements--;
					return;
				}
				node = node->next;
				nbElement++;
			}
			node->next = new_node;
			if (nbElement > 4)
				mtar_hashtable_rehash(hashtable);
		}
	} else
		hashtable->nodes[index] = new_node;
}

void mtar_hashtable_rehash(struct mtar_hashtable * hashtable) {
	if (!hashtable->allow_rehash)
		return;

	hashtable->allow_rehash = 0;

	unsigned int old_size_node = hashtable->size_node;
	struct mtar_hashtable_node ** old_nodes = hashtable->nodes;
	hashtable->nodes = calloc(sizeof(struct mtar_hashtable_node *), hashtable->size_node << 1);
	hashtable->size_node <<= 1;

	unsigned int i;
	for (i = 0; i < old_size_node; i++) {
		struct mtar_hashtable_node * node = old_nodes[i];
		while (node != NULL) {
			struct mtar_hashtable_node * current_node = node;
			node = node->next;
			current_node->next = 0;

			unsigned int index = current_node->hash % hashtable->size_node;
			mtar_hashtable_put2(hashtable, index, current_node);
		}
	}
	free(old_nodes);

	hashtable->allow_rehash = 1;
}

void * mtar_hashtable_remove(struct mtar_hashtable * hashtable, const void * key) {
	if (hashtable == NULL || key == NULL)
		return NULL;

	unsigned long long hash = hashtable->compupte_hash(key);
	unsigned int index = hash % hashtable->size_node;

	struct mtar_hashtable_node * node = hashtable->nodes[index];
	if (node != NULL && node->hash == hash) {
		hashtable->nodes[index] = node->next;
		void * value = node->value;
		free(node);
		hashtable->nb_elements--;
		return value;
	}

	while (node != NULL && node->next) {
		if (node->next->hash == hash) {
			struct mtar_hashtable_node * current_node = node->next;
			node->next = current_node->next;

			void * value = current_node->value;
			free(current_node);
			hashtable->nb_elements--;
			return value;
		}
		node->next = node->next->next;
	}

	return NULL;
}

void * mtar_hashtable_value(struct mtar_hashtable * hashtable, const void * key) {
	if (hashtable == NULL || key == NULL)
		return NULL;

	unsigned long long hash = hashtable->compupte_hash(key);
	unsigned int index = hash % hashtable->size_node;

	struct mtar_hashtable_node * node = hashtable->nodes[index];
	while (node != NULL) {
		if (node->hash == hash)
			return node->value;
		node = node->next;
	}

	return NULL;
}

void ** mtar_hashtable_values(struct mtar_hashtable * hashtable) {
	if (!hashtable)
		return NULL;

	void ** values = calloc(sizeof(void *), hashtable->nb_elements + 1);
	unsigned int iNode = 0, index = 0;
	while (iNode < hashtable->size_node) {
		struct mtar_hashtable_node * node = hashtable->nodes[iNode];
		while (node != NULL) {
			values[index] = node->value;
			index++;
			node = node->next;
		}
		iNode++;
	}
	values[index] = 0;

	return values;
}

