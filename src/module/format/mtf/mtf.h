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
*  Last modified: Sat, 27 Oct 2012 00:03:11 +0200                           *
\***************************************************************************/

#ifndef __MTAR_FORMAT_MTF_H__
#define __MTAR_FORMAT_MTF_H__

// {,u}int*_t
#include <stdint.h>

#include <mtar/format.h>

struct mtar_format_mtf_tape_address {
	uint16_t size;
	uint16_t offset;
};

/**
 * descriptor block
 *
 * All integers are little endian
 */
struct mtar_format_mtf_descriptor_block {
	enum {
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


struct mtar_format_mtf_dirb {
	uint32_t attributes;
	unsigned char last_modification_time[5];
	unsigned char creation_date[5];
	unsigned char backup_date[5];
	unsigned char last_access_date[5];
	uint32_t directory_id;
	struct mtar_format_mtf_tape_address directory_name;
};

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

struct mtar_format_mtf_sset {
	uint32_t attributes;
	uint16_t password_encryption_algorithm;
	uint16_t software_compress_algotithm;
	uint16_t software_vendor_id;
	uint16_t data_set_number;
	struct mtar_format_mtf_tape_address data_set_name;
	struct mtar_format_mtf_tape_address data_set_description;
	struct mtar_format_mtf_tape_address data_set_password;
	struct mtar_format_mtf_tape_address user_name;
	uint64_t phyisical_block_address;
	unsigned char media_write_date[5];
	uint8_t software_major_version;
	uint8_t software_minor_version;
	int8_t time_zone;
	uint8_t mtf_major_version;
	uint8_t media_catalog_version;
};

struct mtar_format_mtf_tape {
	uint32_t media_family_id;
	uint32_t tape_attribute;
	uint16_t media_sequence_number;
	uint16_t password_encryption_algorithm;
	uint16_t soft_filemark_block_size;
	uint16_t media_based_catalog_type;
	struct mtar_format_mtf_tape_address media_name;
	struct mtar_format_mtf_tape_address media_description;
	struct mtar_format_mtf_tape_address media_password;
	struct mtar_format_mtf_tape_address software_name;
	uint16_t format_logical_block_size;
	uint16_t software_vendor_id;
	unsigned char media_date[5];
	uint8_t mtf_major_version;
};

struct mtar_format_mtf_volb {
	uint32_t attributes;
	struct mtar_format_mtf_tape_address device_name;
	struct mtar_format_mtf_tape_address volume_name;
	struct mtar_format_mtf_tape_address machine_name;
	unsigned char media_write_date[5];
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
	} type;
	uint16_t stream_file_system_attributes;
	uint16_t stream_format_attributes;
	uint64_t stream_length;
	uint16_t data_encryption_algorithm;
	uint16_t data_compression_algorithm;
	uint16_t checksum;
} __attribute__((packed));

struct mtar_format_reader * mtar_format_mtf_new_reader(struct mtar_io_reader * io, const struct mtar_option * option);
struct mtar_format_writer * mtar_format_mtf_new_writer(struct mtar_io_writer * io, const struct mtar_option * option);

#endif

