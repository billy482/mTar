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
*  Last modified: Mon, 02 May 2011 17:04:41 +0200                       *
\***********************************************************************/

// open
#include <fcntl.h>
// snprintf
#include <stdio.h>
// strcpy, strlen, strncpy
#include <string.h>
// mmap
#include <sys/mman.h>
// fstat, mode_t, open
#include <sys/stat.h>
// fstat, open
#include <sys/types.h>
// fstat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/hashtable.h>
#include <mtar/util.h>

static void mtar_file_exit(void);
static void mtar_file_lookup(const char * filename, char * name, ssize_t namelength, const char * id);
static void mtar_file_init(void);

static struct mtar_hashtable * gidCached = 0;
static struct mtar_hashtable * uidCached = 0;


void mtar_file_convert_mode(char * buffer, mode_t mode) {
	strcpy(buffer, "----------");

	if (S_ISDIR(mode))
		buffer[0] = 'd';
	else if (S_ISCHR(mode))
		buffer[0] = 'c';
	else if (S_ISBLK(mode))
		buffer[0] = 'b';
	else if (S_ISFIFO(mode))
		buffer[0] = 'p';
	else if (S_ISLNK(mode))
		buffer[0] = 'l';
	else if (S_ISSOCK(mode))
		buffer[0] = 's';

	if (mode & S_IRUSR)
		buffer[1] = 'r';
	if (mode & S_IWUSR)
		buffer[2] = 'w';
	if (mode & S_ISUID)
		buffer[3] = 'S';
	else if (mode & S_IXUSR)
		buffer[3] = 'x';

	if (mode & S_IRGRP)
		buffer[4] = 'r';
	if (mode & S_IWGRP)
		buffer[5] = 'w';
	if (mode & S_ISGID)
		buffer[6] = 'S';
	else if (mode & S_IXGRP)
		buffer[6] = 'x';

	if (mode & S_IROTH)
		buffer[7] = 'r';
	if (mode & S_IWOTH)
		buffer[8] = 'w';
	if (mode & S_ISVTX)
		buffer[9] = 'T';
	else if (mode & S_IXOTH)
		buffer[9] = 'x';
}

__attribute__((destructor))
void mtar_file_exit() {
	mtar_hashtable_free(gidCached);
	mtar_hashtable_free(uidCached);
}

void mtar_file_gid2name(char * name, ssize_t namelength, gid_t gid) {
	char cid[16];
	snprintf(cid, 16, "%u", gid);

	if (!mtar_hashtable_hasKey(gidCached, cid)) {
		mtar_file_lookup("/etc/group", name, namelength, cid);
		mtar_hashtable_put(gidCached, strdup(cid), strdup(name));
		return;
	}

	char * value = mtar_hashtable_value(gidCached, cid);
	strncpy(name, value, namelength);
}

__attribute__((constructor))
void mtar_file_init(void) {
	gidCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
	uidCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
}

void mtar_file_lookup(const char * filename, char * name, ssize_t namelength, const char * id) {
	int fd = open(filename, O_RDONLY);

	struct stat fs;
	fstat(fd, &fs);

	char * buffer = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);

	size_t l = strlen(id) + 3;
	char cid[l];
	snprintf(cid, l, ":%s:", id);

	char * ptr = strstr(buffer, cid);
	char * rptr = ptr;
	while (rptr > buffer && rptr[-1] != '\n')
		rptr--;

	char * dot = strchr(rptr, ':');
	strncpy(name, rptr, (dot - rptr) < namelength ? dot - rptr : namelength);

	munmap(buffer, fs.st_size);
	close(fd);
}

void mtar_file_uid2name(char * name, ssize_t namelength, uid_t uid) {
	char cid[16];
	snprintf(cid, 16, "%u", uid);

	if (!mtar_hashtable_hasKey(uidCached, cid)) {
		mtar_file_lookup("/etc/passwd", name, namelength, cid);
		mtar_hashtable_put(uidCached, strdup(cid), strdup(name));
		return;
	}

	char * value = mtar_hashtable_value(uidCached, cid);
	strncpy(name, value, namelength);
}

