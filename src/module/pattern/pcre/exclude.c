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
*  Last modified: Tue, 25 Oct 2011 11:52:35 +0200                           *
\***************************************************************************/

// pcre_compile, pcre_exec, pcre_free
#include <pcre.h>
// strlen
#include <string.h>

#include "common.h"

struct mtar_pattern_pcre_exclude {
	pcre * pattern;
	enum mtar_pattern_option option;
};

static void mtar_pattern_pcre_exclude_free(struct mtar_pattern_exclude * ex);
static int mtar_pattern_pcre_exclude_match(struct mtar_pattern_exclude * ex, const char * filename);

static struct mtar_pattern_exclude_ops mtar_pattern_pcre_exclude_ops = {
	.free  = mtar_pattern_pcre_exclude_free,
	.match = mtar_pattern_pcre_exclude_match,
};


void mtar_pattern_pcre_exclude_free(struct mtar_pattern_exclude * ex) {
	if (!ex)
		return;

	struct mtar_pattern_pcre_exclude * self = ex->data;
	pcre_free(self->pattern);
	free(self);
	free(ex);
}

int mtar_pattern_pcre_exclude_match(struct mtar_pattern_exclude * ex, const char * filename) {
	struct mtar_pattern_pcre_exclude * self = ex->data;
	int cap[2];
	return pcre_exec(self->pattern, 0, filename, strlen(filename), 0, 0, cap, 2) > 0;
}

struct mtar_pattern_exclude * mtar_pattern_pcre_new_exclude(const char * pattern, enum mtar_pattern_option option) {
	const char * error = 0;
	int erroroffset = 0;

	struct mtar_pattern_pcre_exclude * self = malloc(sizeof(struct mtar_pattern_pcre_exclude));
	self->pattern = pcre_compile(pattern, 0, &error, &erroroffset, 0);
	self->option = option;

	struct mtar_pattern_exclude * ex = malloc(sizeof(struct mtar_pattern_exclude));
	ex->ops = &mtar_pattern_pcre_exclude_ops;
	ex->data = self;

	return ex;
}

