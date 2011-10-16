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
*  Last modified: Tue, 27 Sep 2011 19:13:36 +0200                           *
\***************************************************************************/

// free, malloc
#include <stdlib.h>
// strlen, strncmp
#include <string.h>

#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

struct mtar_pattern_simple {
	const char * pattern;
	size_t length;
	enum mtar_pattern_option option;
};

static void mtar_pattern_simple_free(struct mtar_pattern * ex);
static void mtar_pattern_simple_init(void) __attribute__((constructor));
static int mtar_pattern_simple_match(struct mtar_pattern * ex, const char * filename);
static struct mtar_pattern * mtar_pattern_simple_new(const char * pattern, enum mtar_pattern_option option);
static void mtar_pattern_simple_show_description(void);

static struct mtar_pattern_ops mtar_pattern_simple_ops = {
	.free  = mtar_pattern_simple_free,
	.match = mtar_pattern_simple_match,
};

static struct mtar_pattern_driver mtar_pattern_simple_driver = {
	.name             = "simple",
	.new              = mtar_pattern_simple_new,
	.show_description = mtar_pattern_simple_show_description,
	.api_version      = MTAR_PATTERN_API_VERSION,
};


void mtar_pattern_simple_free(struct mtar_pattern * ex) {
	if (!ex)
		return;

	free(ex->data);
	free(ex);
}

void mtar_pattern_simple_init() {
	mtar_pattern_register(&mtar_pattern_simple_driver);
}

int mtar_pattern_simple_match(struct mtar_pattern * ex, const char * filename) {
	struct mtar_pattern_simple * self = ex->data;
	return !strncmp(filename, self->pattern, self->length) ? 1 : 0;
}

struct mtar_pattern * mtar_pattern_simple_new(const char * pattern, enum mtar_pattern_option option) {
	struct mtar_pattern_simple * self = malloc(sizeof(struct mtar_pattern_simple));
	self->pattern = pattern;
	self->length = strlen(pattern);
	self->option = option;

	struct mtar_pattern * ex = malloc(sizeof(struct mtar_pattern));
	ex->ops = &mtar_pattern_simple_ops;
	ex->data = self;

	return ex;
}

void mtar_pattern_simple_show_description() {
	mtar_verbose_printf("strcmp based pattern matching\n");
}

