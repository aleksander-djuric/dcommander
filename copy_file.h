/*
 * copy_file.h
 *
 * Function description: Copy / move a file's preserving attributes.
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#ifndef _COPY_FILE_H
#define _COPY_FILE_H

#define READ_TIMEOUT 60000
#define WRITE_TIMEOUT 60000

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef struct {
	WINDOW *stat;
	const char *src;
	const char *dst;
	int move_flag;
	int cp_top;
	int cp_cur;
} cp_state;

typedef void (*cp_callback)(cp_state *s);

int copy_file(WINDOW *stat, const char *src, const char *dst, int move_flag, cp_callback cpcb);

#endif // _COPY_FILE_H
