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
*  Last modified: Sat, 20 Oct 2012 14:10:06 +0200                           *
\***************************************************************************/

// be*toh, htobe*
#include <endian.h>
// open
#include <fcntl.h>
// glob, globfree
#include <glob.h>
// sg_io_hdr_t
#include <scsi/scsi.h>
// minor, stat
#include <sys/types.h>
// sg_io_hdr_t
#include <scsi/sg.h>
// realloc
#include <stdlib.h>
// strcat, strcpy, strrchr
#include <string.h>
// ioctl
#include <sys/ioctl.h>
// open, stat
#include <sys/stat.h>
// open
#include <sys/types.h>
// close, readlink, stat
#include <unistd.h>

#include "tape.h"

struct scsi_request_sense {
	unsigned char error_code:7;						/* Byte 0 Bits 0-6 */
	char valid:1;									/* Byte 0 Bit 7 */
	unsigned char segment_number;					/* Byte 1 */
	unsigned char sense_key:4;						/* Byte 2 Bits 0-3 */
	unsigned char :1;								/* Byte 2 Bit 4 */
	char ili:1;										/* Byte 2 Bit 5 */
	char eom:1;										/* Byte 2 Bit 6 */
	char filemark:1;								/* Byte 2 Bit 7 */
	unsigned char information[4];					/* Bytes 3-6 */
	unsigned char additional_sense_length;			/* Byte 7 */
	unsigned char command_specific_information[4];	/* Bytes 8-11 */
	unsigned char additional_sense_code;			/* Byte 12 */
	unsigned char additional_sense_code_qualifier;	/* Byte 13 */
	unsigned char :8;								/* Byte 14 */
	unsigned char bit_pointer:3;					/* Byte 15 */
	char bpv:1;
	unsigned char :2;
	char command_data :1;
	char sksv:1;
	unsigned char field_data[2];					/* Byte 16,17 */
};

static void mtar_io_tape_scsi_init(void) __attribute__((constructor));
static int mtar_io_tape_scsi_open(int fd);

static struct mtar_io_tape_scsi_device {
	int dev_minor;
	char * scsi_device;
} * mtar_io_tape_scsi_devices = NULL;
static unsigned int mtar_io_tape_scsi_nb_devices = 0;

void mtar_io_tape_scsi_init() {
	glob_t gl = { .gl_offs = 0 };
	glob("/sys/class/scsi_device/*/device/scsi_tape", GLOB_DOOFFS, 0, &gl);

	unsigned int i;
	for (i = 0; i < gl.gl_pathc; i++) {
		char * ptr = strrchr(gl.gl_pathv[i], '/');
		*ptr = '\0';

		char link[256];
		char path[256];
		strcpy(path, gl.gl_pathv[i]);
		strcat(path, "/generic");
		ssize_t length = readlink(path, link, 256);
		link[length] = '\0';

		char scsi_device[12];
		ptr = strrchr(link, '/');
		strcpy(scsi_device, "/dev");
		strcat(scsi_device, ptr);

		strcpy(path, gl.gl_pathv[i]);
		strcat(path, "/tape");
		length = readlink(path, link, 256);
		link[length] = '\0';

		char device[12];
		ptr = strrchr(link, '/') + 1;
		strcpy(device, "/dev/n");
		strcat(device, ptr);

		struct stat st;
		stat(device, &st);

		mtar_io_tape_scsi_devices = realloc(mtar_io_tape_scsi_devices, (mtar_io_tape_scsi_nb_devices + 1) * sizeof(struct mtar_io_tape_scsi_device));
		mtar_io_tape_scsi_devices[mtar_io_tape_scsi_nb_devices].dev_minor = minor(st.st_rdev) & 0x7F;
		mtar_io_tape_scsi_devices[mtar_io_tape_scsi_nb_devices].scsi_device = strdup(scsi_device);
		mtar_io_tape_scsi_nb_devices++;
	}

	globfree(&gl);
}

int mtar_io_tape_scsi_open(int fd) {
	struct stat st;
	int failed = fstat(fd, &st);
	if (failed)
		return failed;

	int minor = minor(st.st_rdev) & 0x7F;
	unsigned int i;
	for (i = 0; i < mtar_io_tape_scsi_nb_devices; i++) {
		const struct mtar_io_tape_scsi_device * dev = mtar_io_tape_scsi_devices + i;

		if (minor == dev->dev_minor)
			return open(dev->scsi_device, O_RDWR);
	}

	return -1;
}

int mtar_io_tape_scsi_read_capacity(int fd, ssize_t * total_free, ssize_t * total) {
	if (!total_free && !total)
		return 1;

	int fd_sg = mtar_io_tape_scsi_open(fd);
	if (fd_sg < 0)
		return 3;

	struct {
		unsigned char page_code:6;
		unsigned char reserved0:2;
		unsigned char reserved1;
		unsigned short page_length;
		struct {
			unsigned short parameter_code;
			unsigned char list_parameter:1;
			unsigned char list_binary:1;
			unsigned char tmc:2;
			unsigned char etc:1;
			unsigned char tsd:1;
			unsigned char ds:1;
			unsigned char disable_update:1;
			unsigned char parameter_length;
			unsigned int value;
		} values[4];
	} result;

	struct {
		unsigned char operation_code;
		unsigned char saved_paged:1;
		unsigned char parameter_pointer_control:1;
		unsigned char reserved0:3;
		unsigned char obsolete:3;
		unsigned char page_code:6;
		enum {
			page_control_max_value = 0x0,
			page_control_current_value = 0x1,
			page_control_max_value2 = 0x2, // same as page_control_max_value ?
			page_control_power_on_value = 0x3,
		} page_control:2;
		unsigned char reserved1[2];
		unsigned short parameter_pointer; // must be a bigger endian integer
		unsigned short allocation_length; // must be a bigger endian integer
		unsigned char control;
	} __attribute__((packed)) command = {
		.operation_code = 0x4D,
		.saved_paged = 0,
		.parameter_pointer_control = 0,
		.page_code = 0x31,
		.page_control = page_control_current_value,
		.parameter_pointer = 0,
		.allocation_length = htobe16(sizeof(result)),
		.control = 0,
	};

	struct scsi_request_sense sense;
	sg_io_hdr_t header;
	memset(&header, 0, sizeof(header));
	memset(&sense, 0, sizeof(sense));
	memset(&result, 0, sizeof(result));

	header.interface_id = 'S';
	header.cmd_len = sizeof(command);
	header.mx_sb_len = sizeof(sense);
	header.dxfer_len = sizeof(result);
	header.cmdp = (unsigned char *) &command;
	header.sbp = (unsigned char *) &sense;
	header.dxferp = (unsigned char *) &result;
	header.timeout = 60000;
	header.dxfer_direction = SG_DXFER_FROM_DEV;

	int status = ioctl(fd_sg, SG_IO, &header);

	close(fd_sg);

	if (status)
		return 2;

	result.page_length = be16toh(result.page_length);
	int i;
	for (i = 0; i < 4; i++) {
		result.values[i].parameter_code = be16toh(result.values[i].parameter_code);
		result.values[i].value = be32toh(result.values[i].value);
	}

	if (total_free)
		*total_free = result.values[0].value << 20;

	if (total)
		*total = result.values[2].value << 20;

	return 0;
}

int mtar_io_tape_scsi_read_position(int fd, off_t * position) {
	if (!position)
		return 1;

	int fd_sg = mtar_io_tape_scsi_open(fd);
	if (fd_sg < 0)
		return 3;

	struct {
		unsigned char operation_code;
		unsigned char service_action:5;
		unsigned char obsolete:3;
		unsigned char reserved[5];
		unsigned short parameter_length;	// must be a bigger endian integer
		unsigned char control;
	} __attribute__((packed)) command = {
		.operation_code = 0x34,
		.service_action = 0,
		.parameter_length = 0,
		.control = 0,
	};

	struct {
		unsigned char reserved0:1;
		unsigned char perr:1;
		unsigned char block_position_unknown:1;
		unsigned char reserved1:1;
		unsigned char byte_count_unknown:1;
		unsigned char block_count_unknown:1;
		unsigned char end_of_partition:1;
		unsigned char begin_of_partition:1;
		unsigned char partition_number;
		unsigned char reserved2[2];
		unsigned int first_block_location;
		unsigned int last_block_location;
		unsigned char reserved3;
		unsigned int number_of_blocks_in_buffer;
		unsigned int number_of_bytes_in_buffer;
	} __attribute__((packed)) result;

	struct scsi_request_sense sense;
	sg_io_hdr_t header;
	memset(&header, 0, sizeof(header));
	memset(&sense, 0, sizeof(sense));

	header.interface_id = 'S';
	header.cmd_len = sizeof(command);
	header.mx_sb_len = sizeof(sense);
	header.dxfer_len = sizeof(result);
	header.cmdp = (unsigned char *) &command;
	header.sbp = (unsigned char *) &sense;
	header.dxferp = &result;
	header.timeout = 60000;
	header.dxfer_direction = SG_DXFER_FROM_DEV;

	int status = ioctl(fd_sg, SG_IO, &header);

	close(fd_sg);

	if (status)
		return 2;

	*position = be32toh(result.first_block_location);

	return 0;
}

