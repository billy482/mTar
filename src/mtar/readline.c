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
*  Last modified: Fri, 25 May 2012 10:10:11 +0200                           *
\***************************************************************************/

// free, malloc, realloc
#include <stdlib.h>
// memmove, strchr, strdup
#include <string.h>

#include <mtar/io.h>
#include <mtar/readline.h>

struct mtar_readline {
	struct mtar_io_in * io;
	char * buffer;
	ssize_t buffer_length;
	ssize_t buffer_used;
	char delimiter;
	ssize_t blocksize;
};


void mtar_readline_free(struct mtar_readline * rl) {
	if (!rl)
		return;

	rl->io->ops->free(rl->io);
	free(rl->buffer);
	free(rl);
}

char * mtar_readline_getline(struct mtar_readline * rl) {
	if (!rl)
		return 0;

	char * pos = 0;

	while (!pos) {
		char * end = rl->buffer + rl->buffer_used;

		if (rl->buffer_used > 0) {
			pos = strchr(rl->buffer, rl->delimiter);

			if (pos && pos < end) {
				*pos = '\0';
				pos++;

				char * line = strdup(rl->buffer);

				memmove(rl->buffer, pos, end - pos);
				rl->buffer_used -= pos - rl->buffer;

				return line;
			}
		}

		ssize_t needRead = rl->buffer_length - rl->buffer_used;
		if ((needRead << 1) < rl->buffer_length) {
			rl->buffer_length <<= 1;
			rl->buffer = realloc(rl->buffer, rl->buffer_length);
			needRead = rl->buffer_length - rl->buffer_used;
		}

		if (needRead > rl->blocksize)
			needRead = rl->blocksize;

		ssize_t nb_read = rl->io->ops->read(rl->io, rl->buffer + rl->buffer_used, needRead - 1);

		if (nb_read < 0)
			return 0;

		if (nb_read == 0 && rl->buffer_used) {
			char * line = strdup(rl->buffer);
			rl->buffer_used = 0;
			return line;
		}

		if (nb_read == 0)
			return 0;

		rl->buffer_used += nb_read;
		rl->buffer[rl->buffer_used] = '\0';

		pos = 0;
	}

	return 0;
}

struct mtar_readline * mtar_readline_new(struct mtar_io_in * in, char delimiter) {
	struct mtar_readline * rl = malloc(sizeof(struct mtar_readline));
	rl->io = in;
	rl->buffer = malloc(128);
	rl->buffer_length = 128;
	rl->buffer_used = 0;
	rl->delimiter = delimiter;
	rl->blocksize = rl->io->ops->block_size(rl->io);
	return rl;
}

