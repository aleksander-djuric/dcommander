/*
 * winds.h
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#ifndef _WINDS_H
#define _WINDS_H

void ncstart();
void draw_help(WINDOW *help);
void draw_actwin1(WINDOW *actwin, char *caption, char *dst);
void draw_actwin2(WINDOW *actwin, char *caption, char *src, char *dst);
void draw_errwin(WINDOW *errwin, char *caption, char *desc);
void draw_menubar(WINDOW *menu, int size);
void draw_statbar(WINDOW *status, const char *fmt, ...);
int draw_shell(WINDOW *sshell, char *path, char *command);

#endif // _WINDS_H
