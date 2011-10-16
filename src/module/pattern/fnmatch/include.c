/***************************************************************************\
*                                  ______                                   *
*                             __ _/_  __/__ _____                           *
*                            /  ' \/ / / _ `/ __/                           *
*                           /_/_/_/_/  \_,_/_/                              *
*                                                                           *
*  -----------------------------------------------------------------------  *
*  This file is a part of mTar                                              *
*                                                                           *
*  mTar is free software; you can redistribute it and/or                    *
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
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Mon, 10 Oct 2011 22:31:50 +0200                           *
\***************************************************************************/

// fnmatch
#include <fnmatch.h>
// free, malloc
#include <stdlib.h>

#include "common.h"

struct mtar_pattern_fnmatch_include {
	const char * pattern;
	enum mtar_pattern_option option;
};

void mtar_pattern_fnmatch_include_free(struct mtar_pattern_include * pattern);
int mtar_pattern_fnmatch_include_has_next(struct mtar_pattern_include * pattern);
void mtar_pattern_fnmatch_include_next(struct mtar_pattern_include * pattern, char * filename, size_t length);

static struct mtar_pattern_include_ops mtar_pattern_fnmatch_include_ops = {
	.free  = mtar_pattern_fnmatch_include_free,
};


void mtar_pattern_fnmatch_include_free(struct mtar_pattern_include * pattern) {
	if (!pattern)
		return;

	free(pattern->data);
	free(pattern);
}

struct mtar_pattern_include * mtar_pattern_fnmatch_new_include(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_fnmatch_include * self = malloc(sizeof(struct mtar_pattern_fnmatch_include));
	self->pattern = pattern;
	self->option = option;

	struct mtar_pattern_include * ex = malloc(sizeof(struct mtar_pattern_include));
	ex->ops = &mtar_pattern_fnmatch_include_ops;
	ex->data = self;

	return ex;
}

