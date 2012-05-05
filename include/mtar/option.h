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
*  Last modified: Mon, 10 Oct 2011 21:32:46 +0200                           *
\***************************************************************************/

#ifndef __MTAR_OPTION_H__
#define __MTAR_OPTION_H__

// mode_t
#include <sys/types.h>

#include "function.h"

struct mtar_pattern_exclude;
struct mtar_pattern_include;
struct mtar_pattern_tag;

struct mtar_option {
	// main operation mode
	mtar_function_f doWork;

	// overwrite control
	char verify;

	// handling of file attributes
	enum mtar_option_atime {
		MTAR_OPTION_ATIME_NONE,
		MTAR_OPTION_ATIME_REPLACE,
		MTAR_OPTION_ATIME_SYSTEM,
	} atime_preserve;
	const char * group;
	mode_t mode;
	const char * owner;

	// device selection and switching
	const char * filename;

	// device blocking
	int block_factor;

	// archive format selection
	const char * format;
	const char * label;

	// compression options
	const char * compress_module;
	int compress_level;

	// local file selections
	struct mtar_pattern_include ** files;
	unsigned int nb_files;
	const char * working_directory;
	struct mtar_pattern_exclude ** excludes;
	unsigned int nb_excludes;
	enum mtar_exclude_option {
		MTAR_EXCLUDE_OPTION_DEFAULT = 0x0,
		MTAR_EXCLUDE_OPTION_BACKUP  = 0x1,
		MTAR_EXCLUDE_OPTION_VCS     = 0x2,
	} exclude_option;
	struct mtar_pattern_tag * exclude_tags;
	unsigned int nb_exclude_tags;
	char delimiter;

	// informative output
	int verbose;

	// mtar specific option
	const char ** plugins;
	unsigned int nb_plugins;
};

#endif

