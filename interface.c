/*
 * interface.c
 *
 * Description: Daily Commander File Manager
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#include "commander.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

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
//	curs_set(0);
}

int draw_help(WINDOW *win) {
	int rc = 0;

	wbkgd(win, COLOR_PAIR(2));
	box(win, 0, 0);

	mvwprintw(win, 1, 2, " __           __");
	mvwprintw(win, 2, 2, "|  \\ _ .|    /   _  _  _  _  _  _| _ _");
	mvwprintw(win, 3, 2, "|__/(_|||\\/  \\__(_)||||||(_|| )(_|(-|");
	mvwprintw(win, 3, 43, "v%s", VERSION);
	mvwprintw(win, 4, 2, "         /");
	mvwprintw(win, 6, 2, "Daily Commander comes with ABSOLUTELY NO WARRANTY.");
	mvwprintw(win, 7, 2, "This is free software, and you are welcome to");
	mvwprintw(win, 8, 2, "redistribute it under terms of GNU General Public");
	mvwprintw(win, 9, 2, "License. Press any key to continue..");

	do { rc = getch(); }
		while (rc < 7 && 128 > rc); // press any key
		
	return rc;
}

void draw_pmtwin(WINDOW *win, char *caption, char *dst) {
	wbkgd(win, COLOR_PAIR(2));
	box(win, 0, 0);
	mvwhline(win, 1, 2, ' ', 50);
	mvwprintw(win, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(win, 2, 1, 0, 52);
	mvwhline(win, 6, 2, ' ', 50);
	mvwprintw(win, 6, POPUP_SIZE/2-18, "Press Enter to confirm, Esc to exit.");
	wattron(win, COLOR_PAIR(1));
	mvwhline(win, 4, 2, ' ', 50);
	mvwprintw(win, 4, 2, "%.50s", dst);
	wattroff(win, COLOR_PAIR(2));
	wrefresh(win);
}

int draw_actwin1(WINDOW *win, char *caption, char *dst) {
	int rc = 0;

	wbkgd(win, COLOR_PAIR(2));
	box(win, 0, 0);
	mvwhline(win, 1, 2, ' ', 50);
	mvwprintw(win, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(win, 2, 1, 0, 52);
	mvwhline(win, 6, 2, ' ', 50);
	mvwprintw(win, 6, POPUP_SIZE/2-18, "Press Enter to confirm, Esc to exit.");
	wattron(win, COLOR_PAIR(1));
	mvwhline(win, 4, 2, ' ', 50);
	mvwprintw(win, 4, 2, "%.50s", dst);
	wattroff(win, COLOR_PAIR(2));
	wrefresh(win);

	do { rc = getch(); }
		while (rc != 10 && 27 != rc); // enter or esc
		
	return rc;
}

int draw_actwin2(WINDOW *win, char *caption, char *src, char *dst) {
	int rc = 0;

	wbkgd(win, COLOR_PAIR(2));
	box(win, 0, 0);
	mvwhline(win, 1, 2, ' ', 50);
	mvwprintw(win, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(win, 2, 1, 0, 52);
	mvwhline(win, 8, 2, ' ', 50);
	mvwprintw(win, 8, POPUP_SIZE/2-18, "Press Enter to confirm, Esc to exit.");
	wattron(win, COLOR_PAIR(1));
	mvwhline(win, 4, 2, ' ', 50);
	mvwprintw(win, 4, 2, "%.50s", src);
	wattroff(win, COLOR_PAIR(2));
	mvwprintw(win, 5, 2, "to");
	wattron(win, COLOR_PAIR(1));
	mvwhline(win, 6, 2, ' ', 50);
	mvwprintw(win, 6, 2, "%.50s", dst);
	wattroff(win, COLOR_PAIR(2));
	wrefresh(win);

	do { rc = getch(); }
		while (rc != 10 && 27 != rc); // enter or esc
		
	return rc;
}

int draw_errwin(WINDOW *win, char *caption, char *desc) {
	int rc = 0;

	wbkgd(win, COLOR_PAIR(4));
	box(win, 0, 0);
	mvwhline(win, 1, 2, ' ', 50);
	mvwprintw(win, 1, POPUP_SIZE/2-strlen(caption)/2, "%s", caption);
	mvwhline(win, 2, 1, 0, 52);
	mvwhline(win, 6, 2, ' ', 50);
	mvwprintw(win, 6, POPUP_SIZE/2-11, "Press any key to exit.");
	wattron(win, COLOR_PAIR(5));
	mvwhline(win, 4, 2, ' ', 50);
	mvwprintw(win, 4, 2, "%.50s", desc);
	wattroff(win, COLOR_PAIR(4));
	wrefresh(win);

	do { rc = getch(); }
		while (rc < 7 && 128 > rc); // press any key
		
	return rc;
}

void draw_menubar(WINDOW *win, int size) {
	const char* buttons[MENU_ITEMS] = { "Help", "", "", "", "Copy", "Move", "Folder+", "Delete", "", "Exit" };
	int bs = size / MENU_ITEMS;
	int br = size % MENU_ITEMS;
	int i, pos;

	wbkgd(win, COLOR_PAIR(2));
	wclear(win);

	for (pos = bs, i = 0; i < MENU_ITEMS; i++, pos += bs) {
		wattron(win, COLOR_PAIR(3));
		wprintw(win, "%2d", i+1);
		wattroff(win, COLOR_PAIR(3));
		waddstr(win, buttons[i]);
		if (br) { pos++; br--; }
		wmove(win, 0, pos);
	}
}

void draw_statbar(WINDOW *win, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	wbkgd(win, COLOR_PAIR(2));
	wclear(win);
	vwprintw(win, fmt, args);

	va_end(args);
}

int draw_execwin(WINDOW *win, char *path, int argc, ...) {
	char buffer[PAGE_SIZE];
	char *argv[EXEC_MAXARGS + 1];
	char *args;
	va_list ap;
	pid_t pid;
	int fd[2];
	int i, rc, size;

	wbkgd(win, COLOR_PAIR(3));
	keypad(win, true);
	scrollok(win, true);

	va_start(ap, argc);
	waddch(win, '>');

	for (i = 0; i < argc && i < EXEC_MAXARGS; i++) {
		args = va_arg(ap, char *);
		argv[i] = args;
		wprintw(win, "%s ", args);
	}

	va_end(ap);
	waddch(win, '\n');

	argv[i] = (char *) 0;
	va_end(ap);
	waddch(win, '\n');

	if (pipe(fd) < 0) return -1;

	if ((pid = fork()) == 0) {
		dup2(fd[1], STDOUT_FILENO); // send stdout to the pipe
		dup2(fd[1], STDERR_FILENO); // send stderr to the pipe
		close(fd[0]); // close reading end in the child
		close(fd[1]); // this descriptor is no longer needed
		return execv(path, argv);
	}

	rc = 0;
	waitpid(pid, &rc, 0);
	if (rc < 0) return -1;

	close(fd[1]); // close the write end of the pipe in the parent

	while ((size = read(fd[0], buffer, sizeof(buffer))))
		waddnstr(win, buffer, size);
	waddch(win, '\n');

	wrefresh(win);
	do { rc = getch(); }
		while (rc < 7 && 128 > rc); // press any key

	return 0;
}
