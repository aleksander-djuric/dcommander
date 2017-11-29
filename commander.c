/*
 * commander.c
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */
#include "commander.h"
#include "interface.h"
#include "browser.h"

int dot_filter(const struct dirent *ent) {
	return strcmp(ent->d_name, ".");
}

int main() {
	WINDOW **dirwin, *actwin1, *actwin2, *errwin;
	WINDOW *status, *menu, *help, *execw;
	char srcbuf[MAX_STR];
	char dstbuf[MAX_STR];
	wstate dirstate[2];
	int active, cmd = 0;
	int exitflag = 0;
	int updtflag = 0;

	setlocale(LC_ALL, "");

	dirwin = calloc(2, sizeof(WINDOW *));
	if (!dirwin) return -1;

	ncstart();

	memset(dirstate, 0, sizeof(dirstate));
	dirstate[0].width = COLS/2 - 1;
	dirstate[0].height = LINES - 2;
	dirstate[0].items = NULL;
	dirstate[1].width = COLS - (COLS/2 - 1);
	dirstate[1].height = LINES - 2;
	dirstate[1].items = NULL;

	// Status bar
	status = newwin(1, COLS, 0, 0);
	draw_statbar(status, "");

	// Bottom bar
	menu = newwin(1, COLS, LINES-1, 0);
	draw_menubar(menu, COLS);

	// Panels
	dirwin[0] = newwin(dirstate[0].height, dirstate[0].width, 1, 0);
	wbkgd(dirwin[0], COLOR_PAIR(1));
	box(dirwin[0], 0, 0);
	dirwin[1] = newwin(dirstate[1].height, dirstate[1].width, 1, COLS/2 - 1);
	wbkgd(dirwin[1], COLOR_PAIR(1));
	box(dirwin[1], 0, 0);

	help = newwin(12, POPUP_SIZE, LINES/2-7, COLS/2-POPUP_SIZE/2); // Help window
	actwin1 = newwin(8, POPUP_SIZE, LINES/2-5, COLS/2-POPUP_SIZE/2); // Action 1 window
	actwin2 = newwin(10, POPUP_SIZE, LINES/2-6, COLS/2-POPUP_SIZE/2); // Action 2 window
	errwin = newwin(8, POPUP_SIZE, LINES/2-5, COLS/2-POPUP_SIZE/2); // Error window
	execw = newwin(LINES-2, COLS, 1, 0); // Exec window

	refresh();
	wrefresh(status);
	wrefresh(dirwin[0]);
	wrefresh(dirwin[1]);
	wrefresh(menu);

	if (getcwd(dirstate[0].path, MAX_STR-1) == NULL)
		snprintf(dirstate[0].path, MAX_STR-1, "/");
	snprintf(dirstate[1].path, MAX_STR-1, "/");

	dirstate[0].count = scandir(dirstate[0].path, &(dirstate[0].items), dot_filter, alphasort);
	dirstate[1].count = scandir(dirstate[1].path, &(dirstate[1].items), dot_filter, alphasort);

	browser(dirwin[0], &dirstate[0], 9, 1);
	browser(dirwin[1], &dirstate[1], 9, 0);

	active = 0;

	do {
		struct stat st;
		char *p;
		pid_t pid;
		int rc, x, y;

		cmd = getch();
		switch (cmd) {
		case KEY_F(1):
			draw_help(help);
			updtflag = 2;
			break;
		case KEY_F(3):
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
			getyx(dirwin[active], y, x);

			if ((pid = fork()) == 0)
				return execl("/usr/bin/mcview", "mcview", dstbuf, (char *)0);
			waitpid(pid, &rc, WNOHANG);
			if (rc < 0) draw_errwin(errwin, "Error", errno);

			wrefresh(status);
			wrefresh(dirwin[0]);
			wrefresh(dirwin[1]);
			wrefresh(menu);
			wmove(dirwin[active], y, x);

			break;
		case KEY_F(4):
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
			getyx(dirwin[active], y, x);

			if ((pid = fork()) == 0)
				return execl("/usr/bin/mcedit", "mcedit", dstbuf, (char *)0);
			waitpid(pid, &rc, WNOHANG);
			if (rc < 0) draw_errwin(errwin, "Errwin", errno);

			wrefresh(status);
			wrefresh(dirwin[0]);
			wrefresh(dirwin[1]);
			wrefresh(menu);
			wmove(dirwin[active], y, x);

			break;
		case KEY_F(5):
			cmd = draw_actwin2(actwin2, "Copy", dirstate[active].items[dirstate[active].choice]->d_name, dirstate[active ^ 1].path);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active ^ 1].path, p);

				if (draw_execwin(execw, "/bin/cp", 4, "cp", "-Rfpv", srcbuf, dstbuf) < 0)
					draw_errwin(errwin, "Errno", errno);
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(6):
			cmd = draw_actwin2(actwin2, "Move", dirstate[active].items[dirstate[active].choice]->d_name, dirstate[active ^ 1].path);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active ^ 1].path, p);

				if (draw_execwin(execw, "/bin/mv", 4, "mv", "-fv", srcbuf, dstbuf) < 0)
					draw_errwin(errwin, "Errno", errno);
				else if (dirstate[active].choice > 0) dirstate[active].choice--;
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(7):
			srcbuf[0] = '\0';
			cmd = draw_pmtwin(actwin1, "Create directory", srcbuf);
			snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, srcbuf);

			if (cmd != 27) {
				if (draw_execwin(execw, "/bin/mkdir", 4, "mkdir", "-vm", "0700", dstbuf) < 0)
					draw_errwin(errwin, "Error", errno);
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(8):
			cmd = draw_actwin1(actwin1, "Delete", dirstate[active].items[dirstate[active].choice]->d_name);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);

				if (draw_execwin(execw, "/bin/rm", 3, "rm", "-rfv", dstbuf) < 0)
					draw_errwin(errwin, "Error", errno);
				else if (dirstate[active].choice > 0) dirstate[active].choice--;
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(10):
			exitflag = 1;
			break;
		case KEY_STAB:
		case 9:
			updtflag = 2;
			active ^= 1;
			break;
		case KEY_ENTER:
		case 10:
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
			if (stat(dstbuf, &st) < 0) break;

			if (S_ISDIR(st.st_mode)) {
				if (lstat(dstbuf, &st) < 0) break;
				strncpy(dirstate[active].path, dstbuf, MAX_STR-1);
				dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);

				if (*p == '.' && *(p + 1) == '.') {
					dirstate[active].choice = dirstate[active].prev;
					dirstate[active].prev = 0;
				} else if (S_ISLNK(st.st_mode)) {
					dirstate[active].prev = 0;
					dirstate[active].choice = 0;
				} else {
					dirstate[active].prev = dirstate[active].choice;
					dirstate[active].choice = 0;
				}
			} else if (st.st_mode & S_IXUSR) {
				if (draw_execwin(execw, dstbuf, 1, p) < 0)
					draw_errwin(errwin, "Error", errno);
			}

			updtflag = 2;
			break;
		case KEY_RESIZE:
			wresize(status, 1, COLS);
			draw_statbar(status, "");
			wresize(menu, 1, COLS);
			mvwin(menu, LINES-1, 0);
			draw_menubar(menu, COLS);

			dirstate[0].width = COLS/2 - 1;
			dirstate[0].height = LINES - 2;
			dirstate[1].width = COLS - (COLS/2 - 1);
			dirstate[1].height = LINES - 2;

			wresize(dirwin[0], dirstate[0].height, dirstate[0].width);
			wresize(dirwin[1], dirstate[1].height, dirstate[1].width);
			wresize(execw, LINES-2, COLS);

			mvwin(dirwin[1], 1, COLS/2 - 1);
			mvwin(help, LINES/2-7, COLS/2-POPUP_SIZE/2);
			mvwin(actwin1, LINES/2-5, COLS/2-POPUP_SIZE/2);
			mvwin(actwin2, LINES/2-6, COLS/2-POPUP_SIZE/2);
			mvwin(errwin, LINES/2-5, COLS/2-POPUP_SIZE/2);

			dirstate[0].choice = dirstate[0].start;
			dirstate[1].choice = dirstate[1].start;

			wrefresh(status);
			wrefresh(menu);
			updtflag = 2;

			break;
		default:
			updtflag = 1;
			break;
		}

		switch (updtflag) {
		case 2:
			while (dirstate[active ^ 1].count < 0) {
				if ((p = strrchr(dirstate[active ^ 1].path, '/'))) *(p + 1) = '\0';
				dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			}
			browser(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
		case 1:
			while (dirstate[active].count < 0) {
				if ((p = strrchr(dirstate[active].path, '/'))) *(p + 1) = '\0';
				dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			}
			browser(dirwin[active], &dirstate[active], cmd, 1);
			wrefresh(dirwin[active]);
		default:
			updtflag = 0;
		}
	} while (!exitflag);

	delwin(help);
	delwin(actwin1);
	delwin(actwin2);
	delwin(errwin);
	delwin(execw);
	free(dirstate[0].items);
	free(dirstate[1].items);
	delwin(dirwin[0]);
	delwin(dirwin[1]);
	delwin(status);
	delwin(menu);
	endwin();

	return 0;
}
