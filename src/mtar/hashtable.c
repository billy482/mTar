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
*  Last modified: Thu, 22 Sep 2011 10:21:37 +0200                           *
\***************************************************************************/

// calloc, free, malloc
#include <stdlib.h>

#include <mtar/hashtable.h>

void mtar_hashtable_put2(struct mtar_hashtable * hashtable, unsigned int index, struct mtar_hashtable_node * new_node);
void mtar_hashtable_rehash(struct mtar_hashtable * hashtable);


void mtar_hashtable_free(struct mtar_hashtable * hashtable) {
	if (!hashtable)
		return;

	unsigned int i;
	for (i = 0; i < hashtable->sizeNode; i++) {
		struct mtar_hashtable_node * ptr = hashtable->nodes[i];
		while (ptr != 0) {
			struct mtar_hashtable_node * tmp = ptr;
			ptr = ptr->next;
			if (hashtable->releaseKeyValue)
				hashtable->releaseKeyValue(tmp->key, tmp->value);
			tmp->key = tmp->value = tmp->next = 0;
			free(tmp);
		}
	}
	free(hashtable->nodes);
	hashtable->nodes = 0;
	hashtable->computeHash = 0;

	free(hashtable);
}

short mtar_hashtable_hasKey(struct mtar_hashtable * hashtable, const void * key) {
	if (!hashtable || !key)
		return 0;

	unsigned long long hash = hashtable->computeHash(key);
	unsigned int index = hash % hashtable->sizeNode;

	const struct mtar_hashtable_node * node = hashtable->nodes[index];
	while (node) {
		if (node->hash == hash)
			return 1;
		node = node->next;
	}

	return 0;
}

const void ** mtar_hashtable_keys(struct mtar_hashtable * hashtable) {
	if (!hashtable)
		return 0;

	const void ** keys = calloc(sizeof(void *), hashtable->nbElements + 1);
	unsigned int iNode = 0, index = 0;
	while (iNode < hashtable->sizeNode) {
		struct mtar_hashtable_node * node = hashtable->nodes[iNode];
		while (node) {
			keys[index] = node->key;
			index++;
			node = node->next;
		}
		iNode++;
	}
	keys[index] = 0;

	return keys;
}

struct mtar_hashtable * mtar_hashtable_new(mtar_hashtable_computeHash_f computeHash) {
	return mtar_hashtable_new2(computeHash, 0);
}

struct mtar_hashtable * mtar_hashtable_new2(mtar_hashtable_computeHash_f computeHash, mtar_hashtable_free_f releaseKeyValue) {
	if (!computeHash)
		return 0;

	struct mtar_hashtable * l_hash = malloc(sizeof(struct mtar_hashtable));

	l_hash->nodes = calloc(sizeof(struct mtar_hashtable_node *), 16);
	l_hash->nbElements = 0;
	l_hash->sizeNode = 16;
	l_hash->allowRehash = 1;
	l_hash->computeHash = computeHash;
	l_hash->releaseKeyValue = releaseKeyValue;

	return l_hash;
}

void mtar_hashtable_put(struct mtar_hashtable * hashtable, void * key, void * value) {
	if (!hashtable || !key || !value)
		return;

	unsigned long long hash = hashtable->computeHash(key);
	unsigned int index = hash % hashtable->sizeNode;

	struct mtar_hashtable_node * new_node = malloc(sizeof(struct mtar_hashtable_node));
	new_node->hash = hash;
	new_node->key = key;
	new_node->value = value;
	new_node->next = 0;

	mtar_hashtable_put2(hashtable, index, new_node);
	hashtable->nbElements++;
}

void mtar_hashtable_put2(struct mtar_hashtable * hashtable, unsigned int index, struct mtar_hashtable_node * new_node) {
	struct mtar_hashtable_node * node = hashtable->nodes[index];
	if (node) {
		if (node->hash == new_node->hash) {
			if (hashtable->releaseKeyValue && node->key != new_node->key && node->value != new_node->value)
				hashtable->releaseKeyValue(node->key, node->value);
			hashtable->nodes[index] = new_node;
			new_node->next = node->next;
			free(node);
			hashtable->nbElements--;
		} else {
			short nbElement = 1;
			while (node->next) {
				if (node->next->hash == new_node->hash) {
					struct mtar_hashtable_node * next = node->next;
					if (hashtable->releaseKeyValue && next->key != new_node->key && next->value != new_node->value)
						hashtable->releaseKeyValue(next->key, next->value);
					new_node->next = next->next;
					node->next = new_node;
					free(next);
					hashtable->nbElements--;
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
	if (!hashtable->allowRehash)
		return;

	hashtable->allowRehash = 0;

	unsigned int old_sizeNode = hashtable->sizeNode;
	struct mtar_hashtable_node ** old_nodes = hashtable->nodes;
	hashtable->nodes = calloc(sizeof(struct mtar_hashtable_node *), hashtable->sizeNode << 1);
	hashtable->sizeNode <<= 1;

	unsigned int i;
	for (i = 0; i < old_sizeNode; i++) {
		struct mtar_hashtable_node * node = old_nodes[i];
		while (node) {
			struct mtar_hashtable_node * current_node = node;
			node = node->next;
			current_node->next = 0;

			unsigned int index = current_node->hash % hashtable->sizeNode;
			mtar_hashtable_put2(hashtable, index, current_node);
		}
	}
	free(old_nodes);

	hashtable->allowRehash = 1;
}

void * mtar_hashtable_remove(struct mtar_hashtable * hashtable, const void * key) {
	if (!hashtable || !key)
		return 0;

	unsigned long long hash = hashtable->computeHash(key);
	unsigned int index = hash % hashtable->sizeNode;

	struct mtar_hashtable_node * node = hashtable->nodes[index];
	if (node && node->hash == hash) {
		hashtable->nodes[index] = node->next;
		void * value = node->value;
		free(node);
		hashtable->nbElements--;
		return value;
	}
	while (node->next) {
		if (node->next->hash == hash) {
			struct mtar_hashtable_node * current_node = node->next;
			node->next = current_node->next;

			void * value = current_node->value;
			free(current_node);
			hashtable->nbElements--;
			return value;
		}
		node->next = node->next->next;
	}

	return 0;
}

void * mtar_hashtable_value(struct mtar_hashtable * hashtable, const void * key) {
	if (!hashtable || !key)
		return 0;

	unsigned long long hash = hashtable->computeHash(key);
	unsigned int index = hash % hashtable->sizeNode;

	struct mtar_hashtable_node * node = hashtable->nodes[index];
	while (node) {
		if (node->hash == hash)
			return node->value;
		node = node->next;
	}

	return 0;
}

void ** mtar_hashtable_values(struct mtar_hashtable * hashtable) {
	if (!hashtable)
		return 0;

	void ** values = calloc(sizeof(void *), hashtable->nbElements + 1);
	unsigned int iNode = 0, index = 0;
	while (iNode < hashtable->sizeNode) {
		struct mtar_hashtable_node * node = hashtable->nodes[iNode];
		while (node) {
			values[index] = node->value;
			index++;
			node = node->next;
		}
		iNode++;
	}
	values[index] = 0;

	return values;
}
