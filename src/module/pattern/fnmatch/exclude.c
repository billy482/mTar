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
*  Last modified: Mon, 10 Oct 2011 21:56:31 +0200                           *
\***************************************************************************/

// fnmatch
#include <fnmatch.h>
// free, malloc
#include <stdlib.h>

#include "common.h"

struct mtar_pattern_fnmatch_exclude {
	const char * pattern;
	enum mtar_pattern_option option;
};

static void mtar_pattern_fnmatch_exclude_free(struct mtar_pattern_exclude * ex);
static int mtar_pattern_fnmatch_exclude_match(struct mtar_pattern_exclude * ex, const char * filename);

static struct mtar_pattern_exclude_ops mtar_pattern_fnmatch_exclude_ops = {
	.free  = mtar_pattern_fnmatch_exclude_free,
	.match = mtar_pattern_fnmatch_exclude_match,
};


void mtar_pattern_fnmatch_exclude_free(struct mtar_pattern_exclude * ex) {
	if (!ex)
		return;

	free(ex->data);
	free(ex);
}

int mtar_pattern_fnmatch_exclude_match(struct mtar_pattern_exclude * ex, const char * filename) {
	struct mtar_pattern_fnmatch_exclude * self = ex->data;
	return !fnmatch(filename, self->pattern, 0) ? 1 : 0;
}

struct mtar_pattern_exclude * mtar_pattern_fnmatch_new_exclude(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_fnmatch_exclude * self = malloc(sizeof(struct mtar_pattern_fnmatch_exclude));
	self->pattern = pattern;
	self->option = option;

	struct mtar_pattern_exclude * ex = malloc(sizeof(struct mtar_pattern_exclude));
	ex->ops = &mtar_pattern_fnmatch_exclude_ops;
	ex->data = self;

	return ex;
}

