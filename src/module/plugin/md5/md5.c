/***********************************************************************\
*                               ______                                  *
*                          __ _/_  __/__ _____                          *
*                         /  ' \/ / / _ `/ __/                          *
*                        /_/_/_/_/  \_,_/_/                             *
*                                                                       *
*  -------------------------------------------------------------------  *
*  This file is a part of mTar                                          *
*                                                                       *
*  mTar is free software; you can redistribute it and/or                *
*  modify it under the terms of the GNU General Public License          *
*  as published by the Free Software Foundation; either version 3       *
*  of the License, or (at your option) any later version.               *
*                                                                       *
*  This program is distributed in the hope that it will be useful,      *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of       *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
*  GNU General Public License for more details.                         *
*                                                                       *
*  You should have received a copy of the GNU General Public License    *
*  along with this program; if not, write to the Free Software          *
*  Foundation, Inc., 51 Franklin Street, Fifth Floor,                   *
*  Boston, MA  02110-1301, USA.                                         *
*                                                                       *
*  -------------------------------------------------------------------  *
*  Copyright (C) 2011, Clercin guillaume <clercin.guillaume@gmail.com>  *
*  Last modified: Tue, 10 May 2011 17:42:42 +0200                       *
\***********************************************************************/

// malloc
#include <stdlib.h>

#include <mtar/plugin.h>
#include <mtar/verbose.h>

static void mtar_pluing_md5_init(void);
struct mtar_plugin * mtar_plugin_md5_new(const struct mtar_option * option);

int md5_addFile(struct mtar_plugin * p, const char * filename);
int md5_endOfFile(struct mtar_plugin * p);
ssize_t md5_write(struct mtar_plugin * p, const void * data, ssize_t length);

static struct mtar_plugin_ops md5_ops = {
	.addFile   = md5_addFile,
	.endOfFile = md5_endOfFile,
	.write     = md5_write,
};

__attribute__((constructor))
void mtar_pluing_md5_init() {
	mtar_plugin_register("md5", mtar_plugin_md5_new);
}

struct mtar_plugin * mtar_plugin_md5_new(const struct mtar_option * option __attribute__((unused))) {
	struct mtar_plugin * plugin = malloc(sizeof(struct mtar_plugin));
	plugin->name = "md5";
	plugin->ops = &md5_ops,
	plugin->data = 0;
	return plugin;
}


int md5_addFile(struct mtar_plugin * p __attribute__((unused)), const char * filename) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "MD5 addFile: %s\n", filename);
	return 0;
}

int md5_endOfFile(struct mtar_plugin * p __attribute__((unused))) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "MD5 end of file\n");
	return 0;
}

ssize_t md5_write(struct mtar_plugin * p __attribute__((unused)), const void * data __attribute__((unused)), ssize_t length) {
	mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "MD5 write data: length => %llu\n", (unsigned long long int) length);
	return 0;
}

