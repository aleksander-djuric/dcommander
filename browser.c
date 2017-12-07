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

void wprintw_m(WINDOW *win, int row, int maxrows, int maxcols, int attrs, char *path, char *name) {
	struct stat st, lst;
	struct tm *time;
	char timestr[16];
	int y;

	if (stat(path, &st) < 0 || lstat(path, &lst) < 0) return;

	if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR))
		attrs |= COLOR_PAIR(6);
	if (attrs) wattron(win, attrs);

	if (S_ISDIR(st.st_mode) && S_ISLNK(lst.st_mode)) waddch(win, '~');
	else if (S_ISDIR(st.st_mode)) waddch(win, '/');
	else if	(S_ISLNK(lst.st_mode)) waddch(win, '@');
	else if (st.st_mode & S_IXUSR) waddch(win, '*');

	wprintw(win, "%s", name);
	if (attrs) wattroff(win, attrs);

	time = localtime(&(st.st_mtime));
	strftime(timestr, 16, "%b %_2d %H:%M", time);
	mvwprintw(win, row, maxcols-13, "%.15s", timestr);
	strftime(timestr, 16, "%Y%m%d%H%M", time);

	if (attrs & A_REVERSE) { // selected item
		y = maxrows + 1;
		mvwprintw(win, y, 1, "%.*s", maxcols-37, path);
		mvwaddch(win, y, maxcols-35, (S_ISDIR(st.st_mode)) ? 'd' : '-');
		mvwaddch(win, y, maxcols-34, (st.st_mode & S_IRUSR) ? 'r' : '-');
		mvwaddch(win, y, maxcols-33, (st.st_mode & S_IWUSR) ? 'w' : '-');
		mvwaddch(win, y, maxcols-32, (st.st_mode & S_IXUSR) ? 'x' : '-');
		mvwaddch(win, y, maxcols-31, (st.st_mode & S_IRGRP) ? 'r' : '-');
		mvwaddch(win, y, maxcols-30, (st.st_mode & S_IWGRP) ? 'w' : '-');
		mvwaddch(win, y, maxcols-29, (st.st_mode & S_IXGRP) ? 'x' : '-');
		mvwaddch(win, y, maxcols-28, (st.st_mode & S_IROTH) ? 'r' : '-');
		mvwaddch(win, y, maxcols-27, (st.st_mode & S_IWOTH) ? 'w' : '-');
		mvwaddch(win, y, maxcols-26, (st.st_mode & S_IXOTH) ? 'x' : '-');
		mvwprintw(win, y, maxcols-24, "%10ld", st.st_size);
		mvwprintw(win, maxrows+1, maxcols-13, "%s", timestr);
	}
}

int browser(WINDOW *dir, wstate *s, int cmd, int active) {
	char path[2*PATH_MAX];
	int i, y, cur, len, lines, attrs;

	if (s->count < 0) return -1;

	cur = s->choice - s->start;
	lines = s->height - 3;

	len = s->count - s->start;
	if (len > lines) len = lines;
	mvwhline(dir, lines+1, 0, ' ', s->width);

	for (i = s->start, y = 1; i < (s->start + lines); i++, y++) {
		if (i < (s->start + len)) {
			wmove(dir, y, 1);
			wclrtoeol(dir);
			snprintf(path, 2*PATH_MAX-1, "%s/%s", s->path, s->items[i]->d_name);

			attrs = (s->start + cur == i && active) ? A_REVERSE : 0;
			wprintw_m(dir, y, lines, s->width, attrs, path, s->items[i]->d_name);
		} else {
			wmove(dir, y, 1);
			wclrtoeol(dir);
		}
		mvwaddch(dir, y, s->width-14, ACS_VLINE);
	}

	box(dir, 0, 0);
	mvwaddch(dir, 0, s->width-14, ACS_TTEE);
	mvwhline(dir, lines, 0, ACS_HLINE, s->width);
	mvwaddch(dir, lines, 0, ACS_LTEE);
	mvwaddch(dir, lines, s->width-14, ACS_BTEE);
	mvwaddch(dir, lines, s->width-1, ACS_RTEE);
	wmove(dir, 0, 1);
	wrefresh(dir);
	lines--;

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
	case KEY_A3: // numpad pgup
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
	case KEY_LEFT:
		i = s->count - s->start;
		if (i > lines) cur = 0;
		else cur = s->start;
		break;
	case KEY_END:
	case KEY_RIGHT:
		i = s->count - s->start;
		if (i > lines) cur = lines - 1;
		else cur = i - 1;
		break;
	default:
		break;
	}

	s->choice = cur + s->start;

	return 0;
}
