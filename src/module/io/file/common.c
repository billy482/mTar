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
*  Last modified: Tue, 23 Oct 2012 22:49:50 +0200                           *
\***************************************************************************/

// errno
#include <errno.h>
// fstat
#include <sys/stat.h>
// fstat
#include <sys/types.h>
// close, fstat
#include <unistd.h>

#include "file.h"

ssize_t mtar_io_file_common_block_size(struct mtar_io_file * self) {
	if (self->fd < 0)
		return 0;

	if (self->block_size > 0)
		return self->block_size;

	struct stat st;
	if (fstat(self->fd, &st))
		return -1;

	return self->block_size = st.st_blksize << 8;
}

int mtar_io_file_common_close(struct mtar_io_file * self) {
	if (self->fd < 0)
		return 0;

	int failed = close(self->fd);
	if (failed)
		self->last_errno = errno;
	else
		self->fd = -1;

	return failed;
}

