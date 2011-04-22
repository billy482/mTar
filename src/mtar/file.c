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
*  Last modified: Fri, 22 Apr 2011 12:11:48 +0200                       *
\***********************************************************************/

// getgrgid
#include <grp.h>
// getpwuid
#include <pwd.h>
// strncpy
#include <string.h>
// getpwuid
#include <sys/types.h>

#include <mtar/file.h>

void mtar_file_gid2name(char * name, ssize_t namelength, gid_t gid) {
	struct group * p = getgrgid(gid);
	strncpy(name, p->gr_name, namelength);
}

void mtar_file_uid2name(char * name, ssize_t namelength, uid_t uid) {
	struct passwd * p = getpwuid(uid);
	strncpy(name, p->pw_name, namelength);
}

