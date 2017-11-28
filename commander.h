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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ncurses.h>
#include <locale.h>
#include <time.h>
#include "config.h"

#define MENU_ITEMS 10
#define POPUP_SIZE 54
#define MAX_STR 1024
#define LOG_SIZE 32768
#define EXEC_MAXARGS 10

typedef struct {
	int start;
	int width;
	int height;
	int choice;
	int prev;
	struct dirent **items;
	int count;
	char path[MAX_STR];
} wstate;

#endif // _COMMANDER_H
