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
*  Last modified: Tue, 03 May 2011 17:30:56 +0200                       *
\***********************************************************************/

// strcmp
#include <string.h>
// calloc, free, realloc
#include <stdlib.h>

#include <mtar/option.h>

#include "loader.h"
#include "plugin.h"

static void mtar_plugin_exit(void);

static struct plugin {
	const char * name;
	mtar_plugin_f plugin;
} * pluginHandlers = 0;
static unsigned int nbPluginHandlers = 0;

static struct mtar_plugin ** plugins = 0;
static unsigned int nbPlugins = 0;

static struct mtar_plugin * plugin_get(const char * name, const struct mtar_option * option);


__attribute__((destructor))
void mtar_plugin_exit() {
	if (nbPluginHandlers > 0)
		free(pluginHandlers);
	pluginHandlers = 0;
}

void mtar_plugin_load(const struct mtar_option * option) {
	if (!option || option->nbPlugins == 0)
		return;

	plugins = calloc(sizeof(struct mtar_plugin *), option->nbPlugins);
	nbPlugins = option->nbPlugins;

	unsigned int i;
	for (i = 0; i < option->nbPlugins; i++)
		plugins[i] = plugin_get(option->plugins[i], option);
}

void mtar_plugin_register(const char * name, mtar_plugin_f f) {
	pluginHandlers = realloc(pluginHandlers, (nbPluginHandlers + 1) * (sizeof(struct plugin)));
	pluginHandlers[nbPluginHandlers].name = name;
	pluginHandlers[nbPluginHandlers].plugin = f;
	nbPluginHandlers++;

	loader_register_ok();
}


struct mtar_plugin * plugin_get(const char * name, const struct mtar_option * option) {
	unsigned int i;
	for (i = 0; i < nbPluginHandlers; i++)
		if (!strcmp(name, pluginHandlers[i].name))
			return pluginHandlers[i].plugin(option);
	if (loader_load("plugin", name))
		return 0;
	for (i = 0; i < nbPluginHandlers; i++)
		if (!strcmp(name, pluginHandlers[i].name))
			return pluginHandlers[i].plugin(option);
	return 0;
}


void mtar_plugin_addFile(const char * filename) {
	unsigned int i;
	for (i = 0; i < nbPlugins; i++)
		plugins[i]->ops->addFile(plugins[i], filename);
}

