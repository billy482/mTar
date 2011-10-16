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
*  Last modified: Tue, 27 Sep 2011 18:35:02 +0200                           *
\***************************************************************************/

// pcre_compile, pcre_exec, pcre_free, pcre_version
#include <pcre.h>
// calloc, free
#include <stdlib.h>
// strlen
#include <string.h>

#include <mtar/option.h>
#include <mtar/pattern.h>
#include <mtar/verbose.h>

struct mtar_pattern_pcre {
	pcre * pattern;
};

static void mtar_pattern_pcre_free(struct mtar_pattern * ex);
static void mtar_pattern_pcre_init(void) __attribute__((constructor));
static int mtar_pattern_pcre_match(struct mtar_pattern * ex, const char * filename);
static struct mtar_pattern * mtar_pattern_pcre_new(const char * pattern, enum mtar_pattern_option option);
static void mtar_pattern_pcre_show_description(void);

static struct mtar_pattern_ops mtar_pattern_pcre_ops = {
	.free  = mtar_pattern_pcre_free,
	.match = mtar_pattern_pcre_match,
};

static struct mtar_pattern_driver mtar_pattern_pcre_driver = {
	.name             = "pcre",
	.new              = mtar_pattern_pcre_new,
	.show_description = mtar_pattern_pcre_show_description,
	.api_version      = MTAR_PATTERN_API_VERSION,
};


void mtar_pattern_pcre_free(struct mtar_pattern * ex) {
	if (!ex)
		return;

	struct mtar_pattern_pcre * self = ex->data;
	pcre_free(self->pattern);
	free(self);
	free(ex);
}

void mtar_pattern_pcre_init() {
	mtar_pattern_register(&mtar_pattern_pcre_driver);
}

int mtar_pattern_pcre_match(struct mtar_pattern * ex, const char * filename) {
	struct mtar_pattern_pcre * self = ex->data;
	int cap[2];
	return pcre_exec(self->pattern, 0, filename, strlen(filename), 0, 0, cap, 2) > 0;
}

struct mtar_pattern * mtar_pattern_pcre_new(const char * pattern, enum mtar_pattern_option option) {
	const char * error = 0;
	int erroroffset = 0;

	struct mtar_pattern_pcre * self = malloc(sizeof(struct mtar_pattern_pcre));
	self->pattern = pcre_compile(pattern, 0, &error, &erroroffset, 0);

	struct mtar_pattern * ex = malloc(sizeof(struct mtar_pattern));
	ex->ops = &mtar_pattern_pcre_ops;
	ex->data = self;

	return ex;
}

void mtar_pattern_pcre_show_description() {
	mtar_verbose_printf("pcre powered exclusion files (using libpcre: v%s)\n", pcre_version());
}

