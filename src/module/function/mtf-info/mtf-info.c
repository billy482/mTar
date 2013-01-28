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
*  Copyright (C) 2013, Clercin guillaume <clercin.guillaume@gmail.com>      *
*  Last modified: Mon, 28 Jan 2013 22:23:31 +0100                           *
\***************************************************************************/

// {,u}int*_t
#include <stdint.h>
// time
#include <time.h>

#include <mtar-function-mtf-info.chcksum>
#include <mtar.version>

#include <mtar/filter.h>
#include <mtar/function.h>
#include <mtar/io.h>
#include <mtar/option.h>
#include <mtar/verbose.h>

struct mtar_format_mtf_tape_address {
	uint16_t size;
	uint16_t offset;
};

struct mtar_format_mtf_descriptor_block {
	enum mtar_format_mtf_descriptor_type {
		mtar_format_mtf_descriptor_block_tape = 0x45504154, // tape descriptor
		mtar_format_mtf_descriptor_block_sset = 0x54455353, // start of data set descriptor
		mtar_format_mtf_descriptor_block_volb = 0x424C4F56, // volume descriptor
		mtar_format_mtf_descriptor_block_dirb = 0x42524944, // directory descriptor
		mtar_format_mtf_descriptor_block_file = 0x454C4946, // file descriptor
		mtar_format_mtf_descriptor_block_cfil = 0x4C494643, // Corrupt object descriptor
		mtar_format_mtf_descriptor_block_espb = 0x42505345, // end of set pad descriptor
		mtar_format_mtf_descriptor_block_eset = 0x54455345, // end of set descriptor
		mtar_format_mtf_descriptor_block_eotm = 0x4D544F45, // end of tape marker descriptor
		mtar_format_mtf_descriptor_block_sfmb = 0x424D4653, // soft filemark descriptor
	} type;
	uint32_t attribute;
	uint16_t offset_to_first_event;
	uint8_t os_id;
	uint8_t os_version;
	uint64_t displayable_size;
	uint64_t format_logical_address;
	uint16_t reserved_for_mbc;
	uint64_t reserved1:48;
	uint32_t control_block_id;
	uint32_t reserved2;
	struct mtar_format_mtf_tape_address os_specific_data;
	enum mtar_format_mtf_string_type {
		mtar_format_mtf_string_type_no_string      = 0x0,
		mtar_format_mtf_string_type_ansi_string    = 0x1,
		mtar_format_mtf_string_type_unicode_string = 0x2, // UTF-16le
	} string_type:8;
	uint8_t reserved3;
	uint16_t header_checksum; // XOR sum of all the fields
} __attribute__((packed));

struct mtar_format_mtf_file {
	uint32_t attributes;
	unsigned char last_modification_time[5];
	unsigned char creation_date[5];
	unsigned char backup_date[5];
	unsigned char last_access_date[5];
	uint32_t directory_id;
	uint32_t file_id;
	struct mtar_format_mtf_tape_address file_name;
};

struct mtar_format_mtf_stream {
	enum {
		mtar_format_mtf_stream_stan = 0x4E415453, // standard data stream
		mtar_format_mtf_stream_pnam = 0x4D414E50, // path name stream
		mtar_format_mtf_stream_fnam = 0x4D414E46, // file name stream
		mtar_format_mtf_stream_csum = 0x4D555343, // checksum stream
		mtar_format_mtf_stream_crpt = 0x54505243, // corrupt stream
		mtar_format_mtf_stream_spad = 0x44415053, // pad to next descriptor block
		mtar_format_mtf_stream_spar = 0x52415053, // sparse data
		mtar_format_mtf_stream_tsmp = 0x50534D54, // media based catalog - type 1
		mtar_format_mtf_stream_tfdd = 0x44446D54, // media based catalog - type 1
		mtar_format_mtf_stream_map2 = 0x32504153, // media based catalog - type 2
		mtar_format_mtf_stream_fdd2 = 0x32444446, // media based catalog - type 2
	} type;
	uint16_t stream_file_system_attributes;
	uint16_t stream_format_attributes;
	uint64_t stream_length;
	uint16_t data_encryption_algorithm;
	uint16_t data_compression_algorithm;
	uint16_t checksum;
} __attribute__((packed));


static int mtar_function_mtf_info(const struct mtar_option * option);
static void mtar_function_mtf_info_init(void) __attribute__((constructor));
static void mtar_function_mtf_info_show_description(void);
static void mtar_function_mtf_info_show_help(void);
static void mtar_function_mtf_info_show_version(void);

static struct mtar_function mtar_function_mtf_info_functions = {
	.name             = "mtf-info",

	.do_work           = mtar_function_mtf_info,

	.show_description = mtar_function_mtf_info_show_description,
	.show_help        = mtar_function_mtf_info_show_help,
	.show_version     = mtar_function_mtf_info_show_version,

	.api_level        = {
		.filter   = 0,
		.format   = MTAR_FORMAT_API_LEVEL,
		.function = MTAR_FUNCTION_API_LEVEL,
		.io       = MTAR_IO_API_LEVEL,
		.mtar     = MTAR_API_LEVEL,
		.pattern  = MTAR_PATTERN_API_LEVEL,
	},
};


static int mtar_function_mtf_info(const struct mtar_option * option) {
	struct mtar_io_reader * reader = mtar_filter_get_reader(option);
	int failed = 0;

	uint32_t nb_files = 0;
	bool ok = false;
	ssize_t offset = 0;

	struct mtar_format_mtf_descriptor_block block;
	static const ssize_t sblock = sizeof(block);

	struct mtar_format_mtf_file file;
	static const ssize_t sfile = sizeof(file);

	struct mtar_format_mtf_stream strm;
	static const ssize_t sstrm = sizeof(strm);

	for (;;) {
		offset = reader->ops->position(reader);
		ssize_t nb_read = reader->ops->read(reader, &block, sblock);
		if (nb_read < 0) {
			mtar_verbose_printf("Error while reading\n");
			failed = 1;
			break;
		}
		if (nb_read == 0)
			break;

		if (block.type != mtar_format_mtf_descriptor_block_file) {
			off_t position = reader->ops->position(reader);
			if (position % 1024 > 0)
				reader->ops->forward(reader, 1024 - position % 1024);
			continue;
		}

		nb_read = reader->ops->read(reader, &file, sfile);
		if (nb_read < 0) {
			mtar_verbose_printf("Error while reading\n");
			failed = 1;
			break;
		}
		if (nb_read == 0)
			break;

		mtar_verbose_printf("Found first file: { dir id: %u, file id: %u } @ %zd\n", file.directory_id, file.file_id, offset & ~0x3FF);
		nb_files++;
		ok = true;

		reader->ops->forward(reader, block.offset_to_first_event - sblock - sfile);

		nb_read = reader->ops->read(reader, &strm, sstrm);
		while (!(strm.type == mtar_format_mtf_stream_stan || strm.type == mtar_format_mtf_stream_spad)) {
			offset = reader->ops->position(reader);
			if (offset % 4 != 0)
				reader->ops->forward(reader, 4 - offset % 4);
			reader->ops->forward(reader, strm.stream_length);

			nb_read = reader->ops->read(reader, &strm, sstrm);
		}

		offset = reader->ops->position(reader);
		if (strm.type == mtar_format_mtf_stream_stan || strm.type == mtar_format_mtf_stream_spad)
			reader->ops->forward(reader, strm.stream_length);
		size_t new_pos = reader->ops->position(reader);
		if (new_pos < offset + strm.stream_length)
			break;

		offset = reader->ops->position(reader);
		if (offset % 1024 != 0)
			reader->ops->forward(reader, 1024 - offset % 1024);

		break;
	}

	if (ok) {
		mtar_verbose_printf("Seeking last file");
		mtar_verbose_flush();

		time_t current = time(NULL);

		for (;;) {
			time_t last = time(NULL);
			if (last > current) {
				mtar_verbose_printf("\rcurrent file: { dir id: %u, file id: %u } @ %zd", file.directory_id, file.file_id, offset);
				mtar_verbose_flush();
				current = last;
			}


			ssize_t nb_read = reader->ops->read(reader, &block, sblock);

			if (nb_read < 0) {
				mtar_verbose_printf("Error while reading\n");
				failed = 1;
				break;
			}
			if (nb_read == 0)
				break;

			if (block.type != mtar_format_mtf_descriptor_block_file) {
				off_t position = reader->ops->position(reader);
				if (position % 1024 > 0)
					reader->ops->forward(reader, 1024 - position % 1024);
				continue;
			}

			nb_read = reader->ops->read(reader, &file, sfile);
			if (nb_read < 0) {
				mtar_verbose_printf("Error while reading\n");
				failed = 1;
				break;
			}
			if (nb_read == 0)
				break;

			nb_files++;

			reader->ops->forward(reader, block.offset_to_first_event - sblock - sfile);

			nb_read = reader->ops->read(reader, &strm, sstrm);
			while (!(strm.type == mtar_format_mtf_stream_stan || strm.type == mtar_format_mtf_stream_spad)) {
				offset = reader->ops->position(reader);
				if (offset % 4 != 0)
					reader->ops->forward(reader, 4 - offset % 4);
				reader->ops->forward(reader, strm.stream_length);

				nb_read = reader->ops->read(reader, &strm, sstrm);
			}

			offset = reader->ops->position(reader);
			if (strm.type == mtar_format_mtf_stream_stan || strm.type == mtar_format_mtf_stream_spad)
				reader->ops->forward(reader, strm.stream_length);
			size_t new_pos = reader->ops->position(reader);
			if (new_pos < offset + strm.stream_length)
				break;

			if (new_pos % 1024 != 0)
				reader->ops->forward(reader, 1024 - new_pos % 1024);
		}

		if (!failed) {
			mtar_verbose_clean();
			mtar_verbose_printf("\rFound last file: { dir id: %u, file id: %u } @ %zd\n", file.directory_id, file.file_id, offset & ~0x3FF);
		}
	}

	reader->ops->close(reader);
	reader->ops->free(reader);

	return failed;
}

static void mtar_function_mtf_info_init() {
	mtar_function_register(&mtar_function_mtf_info_functions);
}

static void mtar_function_mtf_info_show_description() {
	mtar_verbose_print_help("mtf-info: Find id of last file found into archive");
}

static void mtar_function_mtf_info_show_help() {
	mtar_verbose_printf("  Find id of last file found into archive\n");
	mtar_verbose_printf("    -f, --file=ARCHIVE  : use ARCHIVE file or device ARCHIVE\n");
}

static void mtar_function_mtf_info_show_version() {
	mtar_verbose_printf("  mtf-info: Find id of last file found into archive (version: " MTAR_VERSION ")\n");
	mtar_verbose_printf("                 SHA1 of source files: " MTAR_FUNCTION_MTF_INFO_SRCSUM "\n");
}

