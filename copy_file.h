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

typedef enum {
	CP_MODE_COPY,
	CP_MODE_MOVE,
	CP_MODE_REMOVE
} cp_flags;

typedef struct {
	WINDOW *stat;
	char *src;
	char *dst;
	mode_t mode;
	size_t size;
	size_t curpos;
	int func;
} cp_state;

typedef void (*cp_callback)(cp_state *s);

int copy_file(WINDOW *stat, char *src, char *dst, int func, cp_callback cpcb);

#endif // _COPY_FILE_H
