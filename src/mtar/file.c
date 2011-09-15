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
*  Last modified: Thu, 15 Sep 2011 09:55:03 +0200                       *
\***********************************************************************/

// open
#include <fcntl.h>
// sscanf, snprintf
#include <stdio.h>
// free, realloc
#include <stdlib.h>
// strchr, strcpy, strlen, strncpy, strstr
#include <string.h>
// mmap, munmap
#include <sys/mman.h>
// fstat, mode_t, open, S_*
#include <sys/stat.h>
// fstat, open, S_*
#include <sys/types.h>
// close, fstat, S_*
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/filter.h>
#include <mtar/hashtable.h>
#include <mtar/option.h>
#include <mtar/readline.h>
#include <mtar/util.h>

static void mtar_file_exit(void) __attribute__((destructor));
static void mtar_file_lookup(const char * filename, char * name, ssize_t namelength, const char * id);
static void mtar_file_rlookup(const char * filename, const char * name, char * id, ssize_t namelength);
static void mtar_file_init(void) __attribute__((constructor));

static struct mtar_hashtable * mtar_file_gidCached = 0;
static struct mtar_hashtable * mtar_file_groupCached = 0;
static struct mtar_hashtable * mtar_file_uidCached = 0;
static struct mtar_hashtable * mtar_file_userCached = 0;


const char ** mtar_file_add_from_file(const char * filename, const char ** files, unsigned int * nbFiles, struct mtar_option * option) {
	if (!filename || !nbFiles || !option)
		return 0;

	struct mtar_io_in * file = mtar_filter_get_in3(filename, option);
	if (!file)
		return 0;

	struct mtar_readline * rl = mtar_readline_new(file, option->delimiter);
	char * line;
	while ((line = mtar_readline_getline(rl))) {
		if (strlen(line) == 0) {
			free(line);
			continue;
		}

		files = realloc(files, (*nbFiles + 1) * sizeof(char *));
		files[*nbFiles] = line;
		(*nbFiles)++;
	}
	mtar_readline_free(rl);

	return files;
}

void mtar_file_convert_mode(char * buffer, mode_t mode) {
	strcpy(buffer, "----------");

	// file type
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

	// user field
	if (mode & S_IRUSR)
		buffer[1] = 'r';
	if (mode & S_IWUSR)
		buffer[2] = 'w';
	if (mode & S_ISUID)
		buffer[3] = 'S';
	else if (mode & S_IXUSR)
		buffer[3] = 'x';

	// group field
	if (mode & S_IRGRP)
		buffer[4] = 'r';
	if (mode & S_IWGRP)
		buffer[5] = 'w';
	if (mode & S_ISGID)
		buffer[6] = 'S';
	else if (mode & S_IXGRP)
		buffer[6] = 'x';

	// other field
	if (mode & S_IROTH)
		buffer[7] = 'r';
	if (mode & S_IWOTH)
		buffer[8] = 'w';
	if (mode & S_ISVTX)
		buffer[9] = 'T';
	else if (mode & S_IXOTH)
		buffer[9] = 'x';
}

void mtar_file_exit() {
	mtar_hashtable_free(mtar_file_gidCached);
	mtar_hashtable_free(mtar_file_groupCached);
	mtar_hashtable_free(mtar_file_uidCached);
	mtar_hashtable_free(mtar_file_userCached);
}

void mtar_file_gid2name(char * name, ssize_t namelength, gid_t gid) {
	// Compute key
	char cid[16];
	snprintf(cid, 16, "%u", gid);

	if (!mtar_hashtable_hasKey(mtar_file_gidCached, cid)) {
		// if lookup has failed
		mtar_file_lookup("/etc/group", name, namelength, cid);
		mtar_hashtable_put(mtar_file_gidCached, strdup(cid), strdup(name));
		return;
	}

	// lookup has succeeded
	char * value = mtar_hashtable_value(mtar_file_gidCached, cid);
	strncpy(name, value, namelength);
}

gid_t mtar_file_group2gid(const char * group) {
	char gid[8];

	if (!mtar_hashtable_hasKey(mtar_file_userCached, group)) {
		// if lookup has failed
		mtar_file_rlookup("/etc/group", group, gid, 8);
		mtar_hashtable_put(mtar_file_userCached, strdup(group), strdup(gid));
	}

	// lookup has succeeded
	char * value = mtar_hashtable_value(mtar_file_userCached, group);
	long gid_value = -1;
	sscanf(value, "%ld", &gid_value);
	return gid_value;
}

void mtar_file_init(void) {
	mtar_file_gidCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
	mtar_file_groupCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
	mtar_file_uidCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
	mtar_file_userCached = mtar_hashtable_new2(mtar_util_compute_hashString, mtar_util_basic_free);
}

void mtar_file_lookup(const char * filename, char * name, ssize_t namelength, const char * id) {
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		return;

	struct stat fs;
	fstat(fd, &fs);

	char * buffer = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);

	size_t l = strlen(id) + 3;
	char cid[l];
	snprintf(cid, l, ":%s:", id);

	char * ptr = strstr(buffer, cid);
	if (ptr) {
		char * rptr = ptr;
		while (rptr > buffer && rptr[-1] != '\n')
			rptr--;

		char * dot = strchr(rptr, ':');
		size_t size = (dot - rptr) < namelength ? dot - rptr : namelength;
		strncpy(name, rptr, size);
		name[size] = '\0';
	} else {
		// lookup failed
		strncpy(name, id, namelength);
	}

	munmap(buffer, fs.st_size);
	close(fd);
}

void mtar_file_rlookup(const char * filename, const char * name, char * id, ssize_t namelength) {
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
		return;

	struct stat fs;
	fstat(fd, &fs);

	char * buffer = mmap(0, fs.st_size, PROT_READ, MAP_SHARED, fd, 0);

	size_t l = strlen(name) + 2;
	char cid[l];
	snprintf(cid, l, "%s:", name);
	char * ptr = strstr(buffer, cid);
	if ((ptr == buffer) || (ptr > buffer && ptr[-1] == '\n')) {
		char * fptr = strchr(ptr, ':') + 3;
		char * dot = strchr(fptr, ':');
		size_t size = (dot - fptr) < namelength ? dot - fptr : namelength;
		strncpy(id, fptr, size);
		id[size] = '\0';
	} else {
		// lookup failed
		strncpy(id, "-1", namelength);
	}

	munmap(buffer, fs.st_size);
	close(fd);
}

void mtar_file_uid2name(char * name, ssize_t namelength, uid_t uid) {
	// Compute key
	char cid[16];
	snprintf(cid, 16, "%u", uid);

	if (!mtar_hashtable_hasKey(mtar_file_uidCached, cid)) {
		// if lookup has failed
		mtar_file_lookup("/etc/passwd", name, namelength, cid);
		mtar_hashtable_put(mtar_file_uidCached, strdup(cid), strdup(name));
		return;
	}

	// lookup has succeeded
	char * value = mtar_hashtable_value(mtar_file_uidCached, cid);
	strncpy(name, value, namelength);
}

uid_t mtar_file_user2uid(const char * user) {
	char uid[8];

	if (!mtar_hashtable_hasKey(mtar_file_userCached, user)) {
		// if lookup has failed
		mtar_file_rlookup("/etc/passwd", user, uid, 8);
		mtar_hashtable_put(mtar_file_userCached, strdup(user), strdup(uid));
	}

	// lookup has succeeded
	char * value = mtar_hashtable_value(mtar_file_userCached, user);
	long uid_value = -1;
	sscanf(value, "%ld", &uid_value);
	return uid_value;
}

