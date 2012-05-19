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
*  Last modified: Sat, 19 May 2012 20:01:05 +0200                           *
\***************************************************************************/

#ifndef __MTAR_FILTER_GZIP_H__
#define __MTAR_FILTER_GZIP_H__

#include <mtar/filter.h>

// See rfc 1952
struct gzip_header {
	unsigned char magic[2];
	unsigned char compression_method;
	enum {
		gzip_flag_none         = 0x00,
		gzip_flag_text         = 0x01,
		gzip_flag_header_crc16 = 0x02,
		gzip_flag_extra_field  = 0x04,
		gzip_flag_name         = 0x08,
		gzip_flag_comment      = 0x10,
	} flag:8;
	int mtime;
	enum {
		gzip_extra_flag_2 = 2, // compressor used maximum compression, slowest algorithm
		gzip_extra_flag_4 = 4, // compressor used fastest algorithm
	} extra_flag:8;
	enum {
		gzip_os_fat_filesystem,
		gzip_os_amiga,
		gzip_os_vms, // or openvms
		gzip_os_unix,
		gzip_os_vm_cms,
		gzip_os_atari_tos,
		gzip_os_hpfs_filesystem, // OS/2, NT
		gzip_os_macintosh,
		gzip_os_zsystem,
		gzip_os_cpm,
		gzip_os_tops_20,
		gzip_os_nfs_filesystem, // NT
		gzip_os_qdos,
		gzip_os_acorn_riscos,

		gzip_os_unknown = 0xFF,
	} os:8;
} __attribute__((packed));

struct mtar_io_in * mtar_filter_gzip_new_in(struct mtar_io_in * io, const struct mtar_option * option);
struct mtar_io_out * mtar_filter_gzip_new_out(struct mtar_io_out * io, const struct mtar_option * option);

#endif

