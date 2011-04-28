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
*  Last modified: Thu, 28 Apr 2011 10:02:38 +0200                       *
\***********************************************************************/

// free, malloc, realloc
#include <stdlib.h>
// snprintf
#include <stdio.h>
// memset, strcpy, strlen, strncpy
#include <string.h>
// bzero
#include <strings.h>
// stat
#include <sys/stat.h>
// stat
#include <sys/types.h>
// time
#include <time.h>
// access, readlink, stat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/io.h>
#include <mtar/verbose.h>

#include "common.h"

struct ustar {
	char filename[100];
	char filemode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char flag;
	char linkname[100];
	char magic[8];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
};

static void ustar_compute_checksum(const void * header, char * checksum);
static void ustar_compute_size(char * csize, ssize_t size);


int mtar_format_ustar_addFile(struct mtar_format * f, const char * filename) {
	if (access(filename, F_OK)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Can access to file: %s\n", filename);
		return 1;
	}
	if (access(filename, R_OK)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Can read file: %s\n", filename);
		return 1;
	}

	struct stat sfile;
	if (lstat(filename, &sfile)) {
		if (stat(filename, &sfile)) {
			mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "An unexpected error occured while getting information about: %s\n", filename);
			return 1;
		}
	}

	ssize_t block_size = 512;
	struct ustar * header = malloc(block_size);
	struct ustar * current_header = header;

	int filename_length = strlen(filename);
	if (filename_length >= 100) {
		block_size += 512 + filename_length - filename_length % 512;
		header = realloc(current_header, block_size);

		bzero(current_header, 1024);
		strcpy(current_header->filename, "././@LongLink");
		memset(current_header->filemode, '0', 7);
		memset(current_header->uid, '0', 7);
		memset(current_header->gid, '0', 7);
		ustar_compute_size(current_header->size, filename_length);
		memset(current_header->mtime, '0', 11);
		current_header->flag = 'L';
		strcpy(current_header->magic, "ustar  ");
		mtar_file_uid2name(current_header->uname, 32, sfile.st_uid);
		mtar_file_gid2name(current_header->gname, 32, sfile.st_gid);

		ustar_compute_checksum(current_header, current_header->checksum);

		strncpy((char *) header + 1, filename, 512);

		current_header += 2;
	}

	bzero(current_header, 512);
	strncpy(header->filename, filename, 100);
	snprintf(current_header->filemode, 8, "%07o", sfile.st_mode & 0777);
	snprintf(current_header->uid, 8, "%07o", sfile.st_uid);
	snprintf(current_header->gid, 8, "%07o", sfile.st_gid);
	snprintf(current_header->mtime, 12, "%0*o", 11, (unsigned int) sfile.st_mtime);

	if (S_ISREG(sfile.st_mode)) {
		ustar_compute_size(current_header->size, sfile.st_size);
		current_header->flag = '0';
	} else if (S_ISLNK(sfile.st_mode)) {
		current_header->flag = '2';
		readlink(filename, current_header->linkname, 100);
	} else if (S_ISCHR(sfile.st_mode)) {
		current_header->flag = '3';
		snprintf(current_header->devmajor, 8, "%07o", (unsigned int) (sfile.st_rdev >> 8));
		snprintf(current_header->devminor, 8, "%07o", (unsigned int) (sfile.st_rdev & 0xFF));
	} else if (S_ISBLK(sfile.st_mode)) {
		current_header->flag = '4';
		snprintf(current_header->devmajor, 8, "%07o", (unsigned int) (sfile.st_rdev >> 8));
		snprintf(current_header->devminor, 8, "%07o", (unsigned int) (sfile.st_rdev & 0xFF));
	} else if (S_ISDIR(sfile.st_mode)) {
		current_header->flag = '5';
	} else if (S_ISFIFO(sfile.st_mode)) {
		current_header->flag = '6';
	}

	strcpy(current_header->magic, "ustar  ");
	mtar_file_uid2name(current_header->uname, 32, sfile.st_uid);
	mtar_file_gid2name(current_header->gname, 32, sfile.st_gid);

	ustar_compute_checksum(current_header, current_header->checksum);

	struct mtar_format_ustar * format = f->data;
	format->position = 0;
	format->size = sfile.st_size;

	ssize_t nbWrite = format->io->ops->write(format->io, header, block_size);

	free(header);

	return block_size != nbWrite;
}

int mtar_format_ustar_addLabel(struct mtar_format * f, const char * label) {
	if (!label) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Label should be defined\n");
		return 1;
	}

	struct ustar * header = malloc(512);
	bzero(header, 512);
	strncpy(header->filename, label, 100);
	snprintf(header->mtime, 12, "%0*o", 11, (unsigned int) time(0));
	header->flag = 'V';

	ustar_compute_checksum(header, header->checksum);

	struct mtar_format_ustar * format = f->data;
	format->position = 0;
	format->size = 0;

	ssize_t nbWrite = format->io->ops->write(format->io, header, 512);

	free(header);

	return 512 != nbWrite;
}

int mtar_format_ustar_addLink(struct mtar_format * f, const char * src, const char * target) {
	if (access(src, F_OK)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "Can access to file: %s\n", src);
		return 1;
	}

	struct stat sfile;
	if (stat(src, &sfile)) {
		mtar_verbose_printf(MTAR_VERBOSE_LEVEL_ERROR, "An unexpected error occured while getting information about: %s\n", src);
		return 1;
	}

	ssize_t block_size = 512;
	struct ustar * header = malloc(block_size);
	struct ustar * current_header = header;

	bzero(current_header, 512);
	strncpy(current_header->filename, src, 100);
	snprintf(current_header->filemode, 8, "%07o", sfile.st_mode & 0777);
	snprintf(current_header->uid, 8, "%07o", sfile.st_uid);
	snprintf(current_header->gid, 8, "%07o", sfile.st_gid);
	snprintf(current_header->mtime, 12, "%0*o", 11, (unsigned int) sfile.st_mtime);
	current_header->flag = '1';
	strncpy(current_header->linkname, target, 100);
	strcpy(current_header->magic, "ustar  ");
	mtar_file_uid2name(current_header->uname, 32, sfile.st_uid);
	mtar_file_gid2name(current_header->gname, 32, sfile.st_gid);

	ustar_compute_checksum(current_header, current_header->checksum);

	struct mtar_format_ustar * format = f->data;
	format->position = 0;
	format->size = 0;

	ssize_t nbWrite = format->io->ops->write(format->io, header, block_size);

	free(header);

	return block_size != nbWrite;
}

int mtar_format_ustar_endOfFile(struct mtar_format * f) {
	struct mtar_format_ustar * format = f->data;

	unsigned short mod = format->position % 512;
	if (mod > 0) {
		char * buffer = malloc(512);
		bzero(buffer, 512 - mod);

		ssize_t nbWrite = format->io->ops->write(format->io, buffer, 512 - mod);
		free(buffer);
		if (nbWrite < 0)
			return -1;
	}

	return 0;
}

void mtar_format_ustar_free(struct mtar_format * f) {
	free(f->data);
	free(f);
}

ssize_t mtar_format_ustar_write(struct mtar_format * f, const void * data, ssize_t length) {
	struct mtar_format_ustar * format = f->data;

	if (format->position >= format->size)
		return 0;

	if (format->position + length > format->size)
		length = format->size - format->position;

	ssize_t nbWrite = format->io->ops->write(format->io, data, length);

	if (nbWrite > 0)
		format->position += nbWrite;

	return nbWrite;
}


void ustar_compute_checksum(const void * header, char * checksum) {
	const unsigned char * ptr = header;

	memset(checksum, ' ', 8);

	unsigned int i, sum = 0;
	for (i = 0; i < 512; i++)
		sum += ptr[i];

	snprintf(checksum, 7, "%06o", sum);
}

void ustar_compute_size(char * csize, ssize_t size) {
	if (size > 077777777) {
		*csize = (char) 0x80;
		unsigned int i;
		for (i = 11; i > 0; i--) {
			csize[i] = size & 0xFF;
			size >>= 8;
		}
	} else {
		snprintf(csize, 12, "%0*lo", 11, size);
	}
}

