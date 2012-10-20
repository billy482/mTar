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
*  Last modified: Sat, 20 Oct 2012 11:02:50 +0200                           *
\***************************************************************************/

// NULL
#include <stddef.h>

#include <mtar/filter.h>
#include <mtar/format.h>
#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/pattern.h>
#include <mtar/plugin.h>


bool mtar_plugin_check(const struct mtar_plugin * plugin) {
	if (plugin == NULL)
		return false;

	if (plugin->filter > 0 && plugin->filter != MTAR_FILTER_API_LEVEL)
		return false;

	if (plugin->format > 0 && plugin->format != MTAR_FORMAT_API_LEVEL)
		return false;

	if (plugin->function > 0 && plugin->function != MTAR_FUNCTION_API_LEVEL)
		return false;

	if (plugin->io > 0 && plugin->io != MTAR_IO_API_LEVEL)
		return false;

	if (plugin->pattern > 0 && plugin->pattern != MTAR_PATTERN_API_LEVEL)
		return false;

	return true;
}

