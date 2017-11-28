/*
 * interface.h
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#ifndef _INTERFACE_H
#define _INTERFACE_H

void ncstart();
int draw_help(WINDOW *win);
int draw_actwin1(WINDOW *win, char *caption, char *dst);
int draw_actwin2(WINDOW *win, char *caption, char *src, char *dst);
int draw_errwin(WINDOW *win, char *caption, char *desc);
void draw_pmtwin(WINDOW *win, char *caption, char *dst);
void draw_menubar(WINDOW *win, int size);
void draw_statbar(WINDOW *win, const char *fmt, ...);
int draw_execwin(WINDOW *win, char *path, int argc, ...);

#endif // _INTERFACE_H
