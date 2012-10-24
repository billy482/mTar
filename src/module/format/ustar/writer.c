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
*  Last modified: Tue, 23 Oct 2012 22:07:47 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// free, malloc, realloc
#include <stdlib.h>
// snprintf
#include <stdio.h>
// memset, strcpy, strlen, strncpy
#include <string.h>
// bzero
#include <strings.h>
// lstat, stat
#include <sys/stat.h>
// lstat, stat
#include <sys/types.h>
// time
#include <time.h>
// access, readlink, stat
#include <unistd.h>

#include <mtar/file.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

#include "ustar.h"

struct mtar_format_ustar_writer {
	struct mtar_io_writer * io;
	off_t position;
	ssize_t size;

	// multi volume
	void * remain_header;
	ssize_t remain_size;

	// handling of file attributes
	const char * group;
	mode_t mode;
	const char * owner;
};


static enum mtar_format_writer_status mtar_format_ustar_writer_add_file(struct mtar_format_writer * f, const char * filename, struct mtar_format_header * header);
static enum mtar_format_writer_status mtar_format_ustar_writer_add_label(struct mtar_format_writer * f, const char * label);
static enum mtar_format_writer_status mtar_format_ustar_writer_add_link(struct mtar_format_writer * f, const char * src, const char * target, struct mtar_format_header * header);
static ssize_t mtar_format_ustar_writer_available_space(struct mtar_format_writer * io);
static ssize_t mtar_format_ustar_writer_block_size(struct mtar_format_writer * f);
static void mtar_format_ustar_writer_compute_checksum(const void * header, char * checksum);
static void mtar_format_ustar_writer_compute_link(struct mtar_format_ustar * header, char * link, const char * filename, ssize_t filename_length, char flag, struct stat * sfile);
static void mtar_format_ustar_writer_compute_size(char * csize, unsigned long long size);
static void mtar_format_ustar_writer_copy(struct mtar_format_ustar_writer * format, struct mtar_format_header * h_to, struct mtar_format_ustar * h_from, struct stat * sfile);
static int mtar_format_ustar_writer_end_of_file(struct mtar_format_writer * f);
static void mtar_format_ustar_writer_free(struct mtar_format_writer * f);
static int mtar_format_ustar_writer_last_errno(struct mtar_format_writer * f);
static ssize_t mtar_format_ustar_writer_next_prefered_size(struct mtar_format_writer * f);
static void mtar_format_ustar_writer_new_volume(struct mtar_format_writer * f, struct mtar_io_writer * file);
static off_t mtar_format_ustar_writer_position(struct mtar_format_writer * io);
static struct mtar_format_reader * mtar_format_ustar_writer_reopen_for_reading(struct mtar_format_writer * f, const struct mtar_option * option);
static int mtar_format_ustar_writer_restart_file(struct mtar_format_writer * f, const char * filename, ssize_t position);
static void mtar_format_ustar_writer_set_mode(struct mtar_format_ustar_writer * format, struct mtar_format_ustar * header, struct stat * sfile);
static void mtar_format_ustar_writer_set_owner_and_group(struct mtar_format_ustar_writer * format, struct mtar_format_ustar * header, struct stat * sfile);
static const char * mtar_format_ustar_writer_skip_leading_slash(const char * str);
static ssize_t mtar_format_ustar_writer_write(struct mtar_format_writer * f, const void * data, ssize_t length);
static enum mtar_format_writer_status mtar_format_ustar_writer_write_header(struct mtar_format_ustar_writer * f, void * data, ssize_t length);

static struct mtar_format_writer_ops mtar_format_ustar_writer_ops = {
	.add_file           = mtar_format_ustar_writer_add_file,
	.add_label          = mtar_format_ustar_writer_add_label,
	.add_link           = mtar_format_ustar_writer_add_link,
	.available_space    = mtar_format_ustar_writer_available_space,
	.block_size         = mtar_format_ustar_writer_block_size,
	.end_of_file        = mtar_format_ustar_writer_end_of_file,
	.free               = mtar_format_ustar_writer_free,
	.last_errno         = mtar_format_ustar_writer_last_errno,
	.next_prefered_size = mtar_format_ustar_writer_next_prefered_size,
	.new_volume         = mtar_format_ustar_writer_new_volume,
	.position           = mtar_format_ustar_writer_position,
	.reopen_for_reading = mtar_format_ustar_writer_reopen_for_reading,
	.restart_file       = mtar_format_ustar_writer_restart_file,
	.write              = mtar_format_ustar_writer_write,
};


struct mtar_format_writer * mtar_format_ustar_new_writer(struct mtar_io_writer * io, const struct mtar_option * option) {
	if (!io)
		return NULL;

	struct mtar_format_ustar_writer * data = malloc(sizeof(struct mtar_format_ustar_writer));
	data->io = io;
	data->position = 0;
	data->size = 0;

	// multi volume
	data->remain_header = NULL;
	data->remain_size = 0;

	// handling of file attributes
	data->mode = option->mode;
	data->owner = option->owner;
	data->group = option->group;

	struct mtar_format_writer * self = malloc(sizeof(struct mtar_format_writer));
	self->ops = &mtar_format_ustar_writer_ops;
	self->data = data;

	return self;
}

static enum mtar_format_writer_status mtar_format_ustar_writer_add_file(struct mtar_format_writer * f, const char * filename, struct mtar_format_header * h_writer) {
	struct stat sfile;
	if (lstat(filename, &sfile)) {
		mtar_verbose_printf("An unexpected error occured while getting information about: %s\n", filename);
		return mtar_format_writer_error;
	}

	ssize_t block_size = 512;
	struct mtar_format_ustar * header = malloc(block_size);
	struct mtar_format_ustar * current_header = header;

	int filename_length = strlen(filename);
	if (S_ISLNK(sfile.st_mode)) {
		char link[257];
		ssize_t link_length = readlink(filename, link, 256);
		link[link_length] = '\0';

		if (filename_length > 100 && link_length > 100) {
			block_size += 2048 + filename_length - filename_length % 512 + link_length - link_length % 512;
			current_header = header = realloc(header, block_size);

			bzero(current_header, block_size - 512);
			mtar_format_ustar_writer_compute_link(current_header, (char *) (current_header + 1), link, link_length, 'K', &sfile);
			mtar_format_ustar_writer_compute_link(current_header + 2, (char *) (current_header + 3), filename, filename_length, 'L', &sfile);

			current_header += 4;
		} else if (filename_length > 100) {
			block_size += 1024 + filename_length - filename_length % 512;
			current_header = header = realloc(header, block_size);

			bzero(current_header, block_size - 512);
			mtar_format_ustar_writer_compute_link(current_header, (char *) (current_header + 1), filename, filename_length, 'L', &sfile);

			current_header += 2;
		} else if (link_length > 100) {
			block_size += 1024 + link_length - link_length % 512;
			current_header = header = realloc(header, block_size);

			bzero(current_header, block_size - 512);
			mtar_format_ustar_writer_compute_link(current_header, (char *) (current_header + 1), link, link_length, 'K', &sfile);

			current_header += 2;
		}
	} else if (filename_length > 100) {
		block_size += 1024 + filename_length - filename_length % 512;
		current_header = header = realloc(header, block_size);

		bzero(current_header, 1024);
		mtar_format_ustar_writer_compute_link(current_header, (char *) (current_header + 1), filename, filename_length, 'L', &sfile);

		current_header += 2;
	}

	struct mtar_format_ustar_writer * format = f->data;
	bzero(current_header, 512);
	strncpy(current_header->filename, mtar_format_ustar_writer_skip_leading_slash(filename), 100);
	mtar_format_ustar_writer_set_mode(format, current_header, &sfile);
	mtar_format_ustar_writer_set_owner_and_group(format, current_header, &sfile);
	snprintf(current_header->mtime, 12, "%0*o", 11, (unsigned int) sfile.st_mtime);

	if (S_ISREG(sfile.st_mode)) {
		mtar_format_ustar_writer_compute_size(current_header->size, sfile.st_size);
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

	mtar_format_ustar_writer_compute_checksum(current_header, current_header->checksum);

	mtar_format_ustar_writer_copy(format, h_writer, header, &sfile);

	format->position = 0;
	format->size = S_ISREG(sfile.st_mode) ? sfile.st_size : 0;

	return mtar_format_ustar_writer_write_header(format, header, block_size);
}

static enum mtar_format_writer_status mtar_format_ustar_writer_add_label(struct mtar_format_writer * f, const char * label) {
	if (label == NULL) {
		mtar_verbose_printf("Label should be defined\n");
		return mtar_format_writer_error;
	}

	struct mtar_format_ustar * header = malloc(512);
	bzero(header, 512);
	strncpy(header->filename, label, 100);
	snprintf(header->mtime, 12, "%0*o", 11, (unsigned int) time(0));
	header->flag = 'V';

	mtar_format_ustar_writer_compute_checksum(header, header->checksum);

	struct mtar_format_ustar_writer * format = f->data;
	format->position = 0;
	format->size = 0;

	return mtar_format_ustar_writer_write_header(format, header, 512);
}

static enum mtar_format_writer_status mtar_format_ustar_writer_add_link(struct mtar_format_writer * f, const char * src, const char * target, struct mtar_format_header * h_writer) {
	struct stat sfile;
	if (lstat(src, &sfile)) {
		mtar_verbose_printf("An unexpected error occured while getting information about: %s\n", src);
		return mtar_format_writer_error;
	}

	ssize_t block_size = 512;
	struct mtar_format_ustar * header = malloc(block_size);
	struct mtar_format_ustar * current_header = header;

	struct mtar_format_ustar_writer * format = f->data;
	bzero(current_header, 512);
	strncpy(current_header->filename, mtar_format_ustar_writer_skip_leading_slash(src), 100);
	mtar_format_ustar_writer_set_mode(format, current_header, &sfile);
	mtar_format_ustar_writer_set_owner_and_group(format, current_header, &sfile);
	snprintf(current_header->mtime, 12, "%0*o", 11, (unsigned int) sfile.st_mtime);
	current_header->flag = '1';
	strncpy(current_header->linkname, mtar_format_ustar_writer_skip_leading_slash(target), 100);
	strcpy(current_header->magic, "ustar  ");

	mtar_format_ustar_writer_compute_checksum(current_header, current_header->checksum);

	mtar_format_ustar_writer_copy(format, h_writer, header, &sfile);
	h_writer->size = 0;

	format->position = 0;
	format->size = 0;

	return mtar_format_ustar_writer_write_header(format, header, block_size);
}

static ssize_t mtar_format_ustar_writer_available_space(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * format = f->data;
	return format->io->ops->available_space(format->io);
}

static void mtar_format_ustar_writer_copy(struct mtar_format_ustar_writer * format, struct mtar_format_header * h_to, struct mtar_format_ustar * h_from, struct stat * sfile) {
	mtar_format_init_header(h_to);

	if (h_from->flag == 'K' || h_from->flag == 'L') {
		const char * filename = (const char *) (h_from + 1);
		h_to->path = strdup(filename);
	} else if (strlen(h_from->filename) > 100) {
		h_to->path = malloc(101);
		strncpy(h_to->path, h_from->filename, 100);
		h_to->path[100] = '\0';
	} else
		h_to->path = strdup(h_from->filename);

	if (h_from->flag == 'K' || h_from->flag == 'L') {
		const char * linkname = (const char *) (h_from + 3);
		h_to->link = strdup(linkname);
	} else if (h_from->flag == 'L') {
		const char * linkname = (const char *) (h_from + 1);
		h_to->link = strdup(linkname);
	} else if (strlen(h_from->linkname) > 100) {
		h_to->link = malloc(101);
		strncpy(h_to->link, h_from->linkname, 100);
		h_to->link[100] = '\0';
	} else
		h_to->link = strdup(h_from->linkname);

	h_to->dev = sfile->st_rdev;
	h_to->size = S_ISREG(sfile->st_mode) ? sfile->st_size : 0;
	h_to->mode = sfile->st_mode;
	h_to->mtime = sfile->st_mtime;

	if (format->owner != NULL) {
		h_to->uid = mtar_file_user2uid(format->owner);
		strcpy(h_to->uname, format->owner);
	} else {
		h_to->uid = sfile->st_uid;
		strcpy(h_to->uname, h_from->uname);
	}

	if (format->group != NULL) {
		h_to->gid = mtar_file_group2gid(format->group);
		strcpy(h_to->gname, format->group);
	} else {
		h_to->gid = sfile->st_gid;
		strcpy(h_to->gname, h_from->gname);
	}

	h_to->is_label = (h_from->flag == 'V');
}

static ssize_t mtar_format_ustar_writer_block_size(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * format = f->data;
	return format->io->ops->block_size(format->io);
}

static void mtar_format_ustar_writer_compute_checksum(const void * header, char * checksum) {
	const unsigned char * ptr = header;

	memset(checksum, ' ', 8);

	unsigned int i, sum = 0;
	for (i = 0; i < 512; i++)
		sum += ptr[i];

	snprintf(checksum, 7, "%06o", sum);
}

static void mtar_format_ustar_writer_compute_link(struct mtar_format_ustar * header, char * link, const char * filename, ssize_t filename_length, char flag, struct stat * sfile) {
	strcpy(header->filename, "././@LongLink");
	memset(header->filemode, '0', 7);
	memset(header->uid, '0', 7);
	memset(header->gid, '0', 7);
	mtar_format_ustar_writer_compute_size(header->size, filename_length);
	memset(header->mtime, '0', 11);
	header->flag = flag;
	strcpy(header->magic, "ustar  ");
	mtar_file_uid2name(header->uname, 32, sfile->st_uid);
	mtar_file_gid2name(header->gname, 32, sfile->st_gid);

	mtar_format_ustar_writer_compute_checksum(header, header->checksum);

	strcpy(link, filename);
}

static void mtar_format_ustar_writer_compute_size(char * csize, unsigned long long size) {
	if (size > 077777777777) {
		*csize = (char) 0x80;
		unsigned int i;
		for (i = 11; i > 0; i--) {
			csize[i] = size & 0xFF;
			size >>= 8;
		}
	} else {
		snprintf(csize, 12, "%0*llo", 11, size);
	}
}

static int mtar_format_ustar_writer_end_of_file(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * format = f->data;

	unsigned short mod = format->position % 512;
	if (mod > 0) {
		char * buffer = malloc(512);
		bzero(buffer, 512 - mod);

		ssize_t nb_write = format->io->ops->write(format->io, buffer, 512 - mod);
		free(buffer);
		if (nb_write < 0)
			return -1;
	}

	return 0;
}

static void mtar_format_ustar_writer_free(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * self = f->data;

	if (self->io != NULL) {
		self->io->ops->close(self->io);
		self->io->ops->free(self->io);
	}

	free(self->remain_header);
	self->remain_header = NULL;

	free(f->data);
	free(f);
}

static int mtar_format_ustar_writer_last_errno(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * format = f->data;
	return format->io->ops->last_errno(format->io);
}

static ssize_t mtar_format_ustar_writer_next_prefered_size(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * format = f->data;
	return format->io->ops->next_prefered_size(format->io);
}

static void mtar_format_ustar_writer_new_volume(struct mtar_format_writer * f, struct mtar_io_writer * file) {
	struct mtar_format_ustar_writer * self = f->data;

	if (self->io != NULL) {
		self->io->ops->close(self->io);
		self->io->ops->free(self->io);
	}

	self->io = file;
}

static off_t mtar_format_ustar_writer_position(struct mtar_format_writer * f) {
	struct mtar_format_ustar_writer * self = f->data;
	return self->position;
}

static struct mtar_format_reader * mtar_format_ustar_writer_reopen_for_reading(struct mtar_format_writer * f, const struct mtar_option * option) {
	struct mtar_format_ustar_writer * format = f->data;
	struct mtar_io_reader * in = format->io->ops->reopen_for_reading(format->io, option);

	if (in != NULL)
		return mtar_format_ustar_new_reader(in, option);

	return 0;
}

static int mtar_format_ustar_writer_restart_file(struct mtar_format_writer * f, const char * filename, ssize_t position) {
	struct stat sfile;
	if (lstat(filename, &sfile)) {
		mtar_verbose_printf("An unexpected error occured while getting information about: %s\n", filename);
		return 1;
	}

	struct mtar_format_ustar_writer * format = f->data;
	if (format->remain_header != NULL) {
		ssize_t nb_write = format->io->ops->write(format->io, format->remain_header, format->remain_size);

		int failed = nb_write != format->remain_size;

		free(format->remain_header);
		format->remain_header = NULL;
		format->remain_size = 0;

		return failed;
	}

	ssize_t block_size = 512;
	struct mtar_format_ustar * header = malloc(block_size);
	struct mtar_format_ustar * current_header = header;
	bzero(current_header, 512);
	strncpy(current_header->filename, mtar_format_ustar_writer_skip_leading_slash(filename), 100);
	mtar_format_ustar_writer_compute_size(current_header->size, sfile.st_size - position);
	current_header->flag = 'M';
	mtar_format_ustar_writer_set_mode(format, current_header, &sfile);
	mtar_format_ustar_writer_set_owner_and_group(format, current_header, &sfile);
	snprintf(current_header->mtime, 12, "%0*o", 11, (unsigned int) sfile.st_mtime);
	strcpy(current_header->magic, "ustar  ");
	mtar_format_ustar_writer_compute_size(current_header->position, position);

	mtar_format_ustar_writer_compute_checksum(current_header, current_header->checksum);

	format->position = 0;
	format->size = sfile.st_size;

	ssize_t nb_write = format->io->ops->write(format->io, header, block_size);

	free(header);

	return block_size != nb_write;
}

static void mtar_format_ustar_writer_set_mode(struct mtar_format_ustar_writer * format, struct mtar_format_ustar * header, struct stat * sfile) {
	if (format->mode)
		snprintf(header->filemode, 8, "%07o", format->mode & 0777);
	else
		snprintf(header->filemode, 8, "%07o", sfile->st_mode & 0777);
}

static void mtar_format_ustar_writer_set_owner_and_group(struct mtar_format_ustar_writer * format, struct mtar_format_ustar * header, struct stat * sfile) {
	if (format->owner != NULL) {
		snprintf(header->uid, 8, "%07o", mtar_file_user2uid(format->owner));
		strncpy(header->uname, format->owner, 32);
	} else {
		snprintf(header->uid, 8, "%07o", sfile->st_uid);
		mtar_file_uid2name(header->uname, 32, sfile->st_uid);
	}

	if (format->group != NULL) {
		snprintf(header->gid, 8, "%07o", mtar_file_group2gid(format->group));
		strncpy(header->gname, format->group, 32);
	} else {
		snprintf(header->gid, 8, "%07o", sfile->st_gid);
		mtar_file_gid2name(header->gname, 32, sfile->st_gid);
	}
}

static const char * mtar_format_ustar_writer_skip_leading_slash(const char * str) {
	if (str == NULL)
		return NULL;

	const char * ptr;
	size_t i = 0, length = strlen(str);
	for (ptr = str; *ptr == '/' && i < length; i++, ptr++);

	return ptr;
}

static ssize_t mtar_format_ustar_writer_write(struct mtar_format_writer * f, const void * data, ssize_t length) {
	struct mtar_format_ustar_writer * format = f->data;

	if (format->position >= format->size)
		return 0;

	if (format->position + length > format->size)
		length = format->size - format->position;

	ssize_t nb_write = format->io->ops->write(format->io, data, length);

	if (nb_write > 0)
		format->position += nb_write;

	return nb_write;
}

static enum mtar_format_writer_status mtar_format_ustar_writer_write_header(struct mtar_format_ustar_writer * f, void * data, ssize_t length) {
	ssize_t available = f->io->ops->available_space(f->io);

	ssize_t nb_write = 0;

	if (available > 0)
		nb_write = f->io->ops->write(f->io, data, length);

	int last_errno = 0;
	if (nb_write < 0) {
		last_errno = f->io->ops->last_errno(f->io);

		if (last_errno != ENOSPC)
			return mtar_format_writer_error;

		nb_write = 0;
	}

	if (nb_write == 0) {
		f->remain_size = length;
		f->remain_header = data;

		f->io->ops->close(f->io);

		return mtar_format_writer_end_of_tape;
	} else if (length > nb_write) {
		f->remain_size = length - nb_write;
		f->remain_header = malloc(f->remain_size);
		memcpy(f->remain_header, data, f->remain_size);
		free(data);

		f->io->ops->close(f->io);

		return mtar_format_writer_end_of_tape;
	}

	free(data);

	return mtar_format_writer_ok;
}

