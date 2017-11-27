/*
 * browser.c
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#include "commander.h"

int wprintw_m(WINDOW *win, int attrs, char *path, char *name) {
	struct stat st, lst;
	int ret;

	if (stat(path, &st) < 0 || lstat(path, &lst) < 0) return -1;

	if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR))
		attrs |= COLOR_PAIR(6);
	if (attrs) wattron(win, attrs);

	if (S_ISDIR(st.st_mode) && S_ISLNK(lst.st_mode)) wprintw(win, "~");
	else if (S_ISDIR(st.st_mode)) wprintw(win, "/");
	else if	(S_ISLNK(lst.st_mode)) wprintw(win, "@");
	else if (st.st_mode & S_IXUSR) wprintw(win, "*");

	ret = wprintw(win, "%s", name);
	if (attrs) wattroff(win, attrs);

	return ret;
}

void browser(WINDOW *dir, wstate *s, int cmd, int active) {
	char path[2*MAX_STR];
	int i, y, cur, len, lines;

	cur = s->choice - s->start;
	lines = s->height - 2;

	len = s->count - s->start;
	if (len > lines) len = lines;

	box(dir, 0, 0);

	for (i = s->start, y = 1; i < (s->start + lines); i++, y++) {
		if (i < (s->start + len)) {
			wmove(dir, y, 1);
			wclrtoeol(dir);
			snprintf(path, 2*MAX_STR-1, "%s/%s", s->path, s->items[i]->d_name);

			if (s->start + cur == i && active)
				wprintw_m(dir, A_REVERSE, path, s->items[i]->d_name);
			else wprintw_m(dir, 0, path, s->items[i]->d_name);
		} else {
			wmove(dir, y, 1);
			wclrtoeol(dir);
		}
	}

	box(dir, 0, 0);
	wmove(dir, 0, 1);
	wrefresh(dir);

	switch (cmd) {
	case KEY_UP:
		if (cur == 0) {
			if (s->start != 0) (s->start)--;
			else break;
		} else cur--;
		break;
	case KEY_DOWN:
		if (cur == (len - 1)) {
			if (s->start < s->count - lines)
				s->start++;
			else break;
		} else cur++;
		break;
	case KEY_PPAGE:
	case KEY_A3:
		if (s->start > lines) s->start -= lines;
		else {
			if (s->start == 0) cur = 0; 
			else s->start = 0;
		}
		break;
	case KEY_NPAGE:
	case KEY_C3: // numpad pgdn
		i = s->count - s->start;
		if (i > lines) {
			s->start += lines;
			i -= lines;
			if (i < lines) cur = 0;
		} else cur = i - 1;
		break;
	case KEY_HOME:
		cur = s->start;
		break;
	case KEY_END:
		i = s->count - s->start;
		if (i > lines) cur = lines - 1;
		else cur = i - 1;
		break;
	default:
		break;
	}

	s->choice = cur + s->start;
}
