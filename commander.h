/*
 * commander.h
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#ifndef _COMMANDER_H
#define _COMMANDER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include <dirent.h>
#include <ncurses.h>
#include <errno.h>
#include <locale.h>
#include "config.h"

#define MENU_ITEMS 10
#define POPUP_SIZE 54
#define LOG_SIZE 32768
#define EXEC_MAXARGS 10

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

typedef struct {
	int start;
	int width;
	int height;
	int choice;
	int prev;
	struct dirent **items;
	int count;
	char path[PATH_MAX];
} wstate;

#endif // _COMMANDER_H
