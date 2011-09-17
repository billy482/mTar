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
*  Last modified: Sat, 17 Sep 2011 12:29:09 +0200                       *
\***********************************************************************/

#ifndef __MTAR_OPTION_H__
#define __MTAR_OPTION_H__

// mode_t
#include <sys/types.h>

#include "function.h"
#include "verbose.h"

struct mtar_exclude_tag;

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
	const char ** files;
	unsigned int nbFiles;
	const char * working_directory;
	const char * exclude_engine;
	const char ** excludes;
	unsigned int nb_excludes;
	enum mtar_exclude_option {
		MTAR_EXCLUDE_OPTION_DEFAULT = 0x0,
		MTAR_EXCLUDE_OPTION_BACKUP  = 0x1,
		MTAR_EXCLUDE_OPTION_VCS     = 0x2,
	} exclude_option;
	struct mtar_exclude_tag * exclude_tags;
	unsigned int nb_exclude_tags;
	char delimiter;

	// informative output
	enum mtar_verbose_level verbose;

	// mtar specific option
	const char ** plugins;
	unsigned int nb_plugins;
};

#endif

