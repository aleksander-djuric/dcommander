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
#include "copy_file.h"

#define CTRL(x) ((x) & 0x1f)

WINDOW *dirwin[2], *actwin1, *actwin2;
WINDOW *status, *menu, *help, *execw;
wstate dirstate[2];

void draw_windows(int cols, int rows);
void delete_windows(void);
int dot_filter(const struct dirent *ent);

void cpcb(cp_state *s) {
	char caption[50];
	int pcnt = (s->curpos * 100. / (s->size + 1));

	if ((pcnt % 10)) return;

	if (s->func == CP_MODE_REMOVE) sprintf(caption, "Removing...");
	else if (s->func == CP_MODE_MOVE) sprintf(caption, "Moving...");
	else if (s->func == CP_MODE_COPY) sprintf(caption, "Copying...");
	else return;

	draw_prgsbar(s->stat, caption, s->src, s->dst, pcnt);
}

int main() {
	cp_callback cpcb_ptr = cpcb;
	char srcbuf[PATH_MAX];
	char dstbuf[PATH_MAX];
	int active, cmd = 0;
	int exitflag = 0;
	int updtflag = 0;

	setlocale(LC_ALL, "");
	setup_term();
	memset(dirstate, 0, sizeof(dirstate));
	draw_windows(COLS, LINES);

	if (getcwd(dirstate[0].path, PATH_MAX-1) == NULL)
		snprintf(dirstate[0].path, PATH_MAX-1, "/");
	snprintf(dirstate[1].path, PATH_MAX-1, "/");

	dirstate[0].count = scandir(dirstate[0].path, &(dirstate[0].items), dot_filter, alphasort);
	dirstate[1].count = scandir(dirstate[1].path, &(dirstate[1].items), dot_filter, alphasort);

	browser(dirwin[0], &dirstate[0], 9, 1);
	browser(dirwin[1], &dirstate[1], 9, 0);

	active = 0;

	do {
		struct stat st;
		char *p;
		pid_t pid;
		int rc;

		cmd = getch();
		switch (cmd) {
		case KEY_F(1):
		case 80:
			draw_help(help);
			updtflag = 2;
			break;
		case KEY_F(3):
		case 82:
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);

			savetty();
			delete_windows();

			if ((pid = fork()) == 0)
				return execl("/usr/bin/mcview", "mcview", dstbuf, (char *)0);
			waitpid(pid, &rc, 0);

			resetty();
			draw_windows(COLS, LINES);

			if (rc < 0) draw_errwin(actwin1, "Viewer exec error", errno);
			updtflag = 2;
			break;
		case KEY_F(4):
		case 83:
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);

			savetty();
			delete_windows();

			if ((pid = fork()) == 0)
				return execl("/usr/bin/mcedit", "mcedit", dstbuf, (char *)0);
			waitpid(pid, &rc, 0);

			resetty();
			draw_windows(COLS, LINES);

			if (rc < 0) draw_errwin(actwin1, "Editor exec error", errno);
			updtflag = 2;
			break;
		case KEY_F(5):
			cmd = draw_actwin2(actwin2, "Copy", dirstate[active].items[dirstate[active].choice]->d_name, dirstate[active ^ 1].path);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active ^ 1].path, p);

				if ((rc = copy_file(actwin2, srcbuf, dstbuf, CP_MODE_COPY, cpcb_ptr)) < 0)
					draw_errwin(actwin1, "Copy error", errno);
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(6):
			cmd = draw_actwin2(actwin2, "Move", dirstate[active].items[dirstate[active].choice]->d_name, dirstate[active ^ 1].path);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active ^ 1].path, p);

				if ((rc = copy_file(actwin2, srcbuf, dstbuf, CP_MODE_MOVE, cpcb_ptr)) < 0)
					draw_errwin(actwin1, "Move error", errno);
				else if (dirstate[active].choice > 0) dirstate[active].choice--;
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(7):
			srcbuf[0] = '\0';
			cmd = draw_pmtwin(actwin1, "Create folder", srcbuf);
			snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, srcbuf);

			if (cmd != 27) {
				if (mkdir(dstbuf, 0700) < 0)
					draw_errwin(actwin1, "Create folder error", errno);
			}

			dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
			dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
			updtflag = 2;
			break;
		case KEY_F(8):
			cmd = draw_actwin1(actwin1, "Delete", dirstate[active].items[dirstate[active].choice]->d_name);

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);

				if (copy_file(actwin2, dstbuf, NULL, CP_MODE_REMOVE, cpcb_ptr) < 0)
					draw_errwin(actwin1, "Delete error", errno);
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
			snprintf(dstbuf, PATH_MAX-1, "%s/%s", dirstate[active].path, p);
			if (stat(dstbuf, &st) < 0) break;

			if (S_ISDIR(st.st_mode)) {
				if (chdir(dstbuf) == 0) {
					if (getcwd(dirstate[active].path, PATH_MAX-1) == NULL)
						strncpy(dirstate[active].path, dstbuf, PATH_MAX-1);
					dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);

					if (*p == '.' && *(p + 1) == '.') {
						dirstate[active].choice = dirstate[active].prev;
						dirstate[active].prev = 0;
					} else if (lstat(dstbuf, &st) == 0 && S_ISLNK(st.st_mode)) {
						dirstate[active].prev = 0;
						dirstate[active].choice = 0;
					} else {
						dirstate[active].prev = dirstate[active].choice;
						dirstate[active].choice = 0;
					}
				} else draw_errwin(actwin1, "Error changing folder", errno);
			} else if (st.st_mode & S_IXUSR) {
				if (draw_execwin(execw, dstbuf, 1, p) < 0)
					draw_errwin(actwin1, "Exec error", errno);
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

			dirstate[0].choice = dirstate[0].start;
			dirstate[1].choice = dirstate[1].start;

			wrefresh(status);
			wrefresh(menu);
			updtflag = 2;
			break;
		case CTRL('o'):
			redrawwin(execw);
			wrefresh(execw);
			while (getch() != CTRL('o'));
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

	free(dirstate[0].items);
	free(dirstate[1].items);
	delete_windows();

	return 0;
}

void draw_windows(int cols, int rows) {
	dirstate[0].width = cols/2 - 1;
	dirstate[0].height = rows - 2;
	dirstate[1].width = cols - (cols/2 - 1);
	dirstate[1].height = rows - 2;

	// Status bar
	status = newwin(1, cols, 0, 0);
	draw_statbar(status, "");

	// Bottom bar
	menu = newwin(1, cols, rows-1, 0);
	draw_menubar(menu, cols);

	// Panels
	dirwin[0] = newwin(dirstate[0].height, dirstate[0].width, 1, 0);
	wbkgd(dirwin[0], COLOR_PAIR(1));
	box(dirwin[0], 0, 0);
	dirwin[1] = newwin(dirstate[1].height, dirstate[1].width, 1, cols/2 - 1);
	wbkgd(dirwin[1], COLOR_PAIR(1));
	box(dirwin[1], 0, 0);

	help = newwin(12, POPUP_SIZE, rows/2-7, cols/2-POPUP_SIZE/2); // Help window
	actwin1 = newwin(8, POPUP_SIZE, rows/2-5, cols/2-POPUP_SIZE/2); // Action 1 window
	actwin2 = newwin(10, POPUP_SIZE, rows/2-6, cols/2-POPUP_SIZE/2); // Action 2 window

	// Log window
	execw = newwin(rows-2, cols, 1, 0);
	wbkgd(execw, COLOR_PAIR(3));
	scrollok(execw, true);

	refresh();
	wrefresh(status);
	wrefresh(dirwin[0]);
	wrefresh(dirwin[1]);
	wrefresh(menu);
}

void delete_windows(void) {
	delwin(help);
	delwin(actwin1);
	delwin(actwin2);
	delwin(execw);
	delwin(dirwin[0]);
	delwin(dirwin[1]);
	delwin(status);
	delwin(menu);
	endwin();
}

int dot_filter(const struct dirent *ent) {
	return strcmp(ent->d_name, ".");
}
