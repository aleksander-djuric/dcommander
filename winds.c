/*
 * winds.c
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#include "commander.h"

void ncstart() {
	initscr();
	keypad(stdscr, true);
	noecho();
	cbreak();

	if (has_colors()) {
		start_color();

		init_pair(1, COLOR_WHITE, COLOR_BLUE);	// edit screen
		init_pair(2, COLOR_BLACK, COLOR_CYAN);	// menus
		init_pair(3, COLOR_CYAN, COLOR_BLACK);	// menu label
		init_pair(4, COLOR_WHITE, COLOR_RED);	// errors
		init_pair(5, COLOR_BLACK, COLOR_CYAN);	// other windows
		init_pair(6, COLOR_YELLOW, COLOR_BLUE);	// selected
	}

	wbkgd(stdscr, COLOR_PAIR(1));
	refresh();
	timeout(0);
	set_escdelay(0);
	curs_set(0);
}

void draw_help(WINDOW *help) {
	wbkgd(help, COLOR_PAIR(2));
	box(help, 0, 0);

	mvwprintw(help, 1, 1, " __           __");
	mvwprintw(help, 2, 1, "|  \\ _ .|    /   _  _  _  _  _  _| _ _");
	mvwprintw(help, 3, 1, "|__/(_|||\\/  \\__(_)||||||(_|| )(_|(-|");
	mvwprintw(help, 4, 1, "         /");
	mvwprintw(help, 6, 1, "CDirector comes with ABSOLUTELY NO WARRANTY. This is");
	mvwprintw(help, 7, 1, "free software, and you are welcome to redistribute");
	mvwprintw(help, 8, 1, "it under terms of GNU General Public License.");
	mvwprintw(help, 10, 1, "Press any key to continue..");
}

void draw_actwin1(WINDOW *actwin, char *caption, char *dst) {
	wbkgd(actwin, COLOR_PAIR(2));
	box(actwin, 0, 0);
	mvwhline(actwin, 1, 2, ' ', 50);
	mvwprintw(actwin, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(actwin, 2, 1, 0, 52);
	mvwhline(actwin, 6, 2, ' ', 50);
	mvwprintw(actwin, 6, POPUP_SIZE/2-18, "Press Enter to confirm, Esc to exit.");
	wattron(actwin, COLOR_PAIR(1));
	mvwhline(actwin, 4, 2, ' ', 50);
	mvwprintw(actwin, 4, 2, "%.50s", dst);
	wattroff(actwin, COLOR_PAIR(2));
}

void draw_actwin2(WINDOW *actwin, char *caption, char *src, char *dst) {
	wbkgd(actwin, COLOR_PAIR(2));
	box(actwin, 0, 0);
	mvwhline(actwin, 1, 2, ' ', 50);
	mvwprintw(actwin, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(actwin, 2, 1, 0, 52);
	mvwhline(actwin, 8, 2, ' ', 50);
	mvwprintw(actwin, 8, POPUP_SIZE/2-18, "Press Enter to confirm, Esc to exit.");
	wattron(actwin, COLOR_PAIR(1));
	mvwhline(actwin, 4, 2, ' ', 50);
	mvwprintw(actwin, 4, 2, "%.50s", src);
	wattroff(actwin, COLOR_PAIR(2));
	mvwprintw(actwin, 5, 2, "to");
	wattron(actwin, COLOR_PAIR(1));
	mvwhline(actwin, 6, 2, ' ', 50);
	mvwprintw(actwin, 6, 2, "%.50s", dst);
	wattroff(actwin, COLOR_PAIR(2));
}

void draw_errwin(WINDOW *errwin, char *caption, char *desc) {
	wbkgd(errwin, COLOR_PAIR(4));
	box(errwin, 0, 0);
	mvwhline(errwin, 1, 2, ' ', 50);
	mvwprintw(errwin, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(errwin, 2, 1, 0, 52);
	mvwhline(errwin, 6, 2, ' ', 50);
	mvwprintw(errwin, 6, POPUP_SIZE/2-11, "Press any key to exit.");
	wattron(errwin, COLOR_PAIR(5));
	mvwhline(errwin, 4, 2, ' ', 50);
	mvwprintw(errwin, 4, 2, "%.50s", desc);
	wattroff(errwin, COLOR_PAIR(4));
}

void draw_menubar(WINDOW *menu, int size) {
	const char* buttons[MENU_ITEMS] = { "Help", "", "", "", "Copy", "Move", "Folder+", "Delete", "", "Exit" };
	int bs = size / MENU_ITEMS;
	int br = size % MENU_ITEMS;
	int i, pos;

	wbkgd(menu, COLOR_PAIR(2));
	wclear(menu);

	for (pos = bs, i = 0; i < MENU_ITEMS; i++, pos += bs) {
		wattron(menu, COLOR_PAIR(3));
		wprintw(menu, "%2d", i+1);
		wattroff(menu, COLOR_PAIR(3));
		waddstr(menu, buttons[i]);
		if (br) { pos++; br--; }
		wmove(menu, 0, pos);
	}
}

void draw_statbar(WINDOW *status, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	wbkgd(status, COLOR_PAIR(2));
	wclear(status);
	vwprintw(status, fmt, args);

	va_end(args);
}

int draw_shell(WINDOW *shell, char *path, char *command) {
	int fd[2];
	pid_t pid;
	char buffer[32768];
	int rc, size;

	wbkgd(shell, COLOR_PAIR(3));
	wprintw(shell, ">%s\n", command);
	keypad(shell, true);
	scrollok(shell, true);

	if (pipe(fd) < 0) return -1;

	if (!(pid = fork())) {
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		return execl(path, command, (char *)0);
	}

	rc = 0;
	waitpid(pid, &rc, 0);
	if (rc < 0) return -1;

	close(fd[1]);

	if ((size = read(fd[0], buffer, sizeof(buffer))))
		wprintw(shell, "%.*s\n", size, buffer);

	return 0;
}
