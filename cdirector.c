/*
 * cdirector.c
 *
 * Description: Ncurses File Manager Example
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

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

#define MENU_ITEMS 10
#define POPUP_SIZE 54
#define MAX_STR 1024

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

void ncstart();
void draw_help(WINDOW *help);
void draw_actwin1(WINDOW *actwin, char *caption, char *dst);
void draw_actwin2(WINDOW *actwin, char *caption, char *src, char *dst);
void draw_errwin(WINDOW *errwin, char *caption, char *desc);
void draw_menubar(WINDOW *menu, int size);
void draw_statbar(WINDOW *status, const char *fmt, ...);
int draw_shell(WINDOW *sshell, char *path, char *command);
int dot_filter(const struct dirent *ent);
int wprintw_m(WINDOW *win, int attrs, char *path, char *name, int maxlen);
void director(WINDOW *dir, wstate *state, int cmd, int active);

int main() {
	WINDOW **dirwin, *status, *menu, *help, *shell;
	WINDOW *actwin1, *actwin2, *errwin;
	char srcbuf[MAX_STR];
	char dstbuf[MAX_STR];
	wstate dirstate[2];
	int i, active, cmd = 0;
	bool exitflag = false;

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

	// Help window
	help = newwin(12, POPUP_SIZE, LINES/2-7, COLS/2-POPUP_SIZE/2);

	// Action 1 window
	actwin1 = newwin(8, POPUP_SIZE, LINES/2-5, COLS/2-POPUP_SIZE/2);

	// Action 2 window
	actwin2 = newwin(10, POPUP_SIZE, LINES/2-6, COLS/2-POPUP_SIZE/2);

	// Error window
	errwin = newwin(8, POPUP_SIZE, LINES/2-5, COLS/2-POPUP_SIZE/2);

	// Subshell window
	shell = newwin(LINES-2, COLS, 1, 0);

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

	director(dirwin[0], &dirstate[0], 9, 1);
	director(dirwin[1], &dirstate[1], 9, 0);

	active = 0;

	do {
		struct stat st;
		char *s, *p;
		pid_t pid;
		int x, y, result;

		cmd = getch();
		switch (cmd) {
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
			wresize(shell, LINES-2, COLS);

			mvwin(dirwin[1], 1, COLS/2 - 1);
			mvwin(help, LINES/2-7, COLS/2-POPUP_SIZE/2);
			mvwin(actwin1, LINES/2-5, COLS/2-POPUP_SIZE/2);
			mvwin(actwin2, LINES/2-6, COLS/2-POPUP_SIZE/2);
			mvwin(errwin, LINES/2-5, COLS/2-POPUP_SIZE/2);

			dirstate[0].choice = dirstate[0].start;
			dirstate[1].choice = dirstate[1].start;
			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);

			wrefresh(dirwin[0]);
			wrefresh(dirwin[1]);
			wrefresh(status);
			wrefresh(menu);
			
			break;
		case KEY_F(1):
			draw_help(help);
			wrefresh(help);
			do { cmd = getch(); }
				while (cmd < 7 && 128 > cmd); // press any key

			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		case KEY_F(5):
			draw_actwin2(actwin2, "Copy", dirstate[active].items[dirstate[active].choice]->d_name,
				dirstate[active ^ 1].path);
			wrefresh(actwin2);
			do { cmd = getch(); }
				while (cmd != 10 && 27 != cmd); // enter or esc

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active ^ 1].path, p);

				result = 0;
				if (!(pid = fork()))
					return execl("/bin/cp", "cp", "-Rfp", srcbuf, dstbuf, (char *)0);
				waitpid(pid, &result, 0);

				if (result < 0) {
					draw_errwin(errwin, "Couldn't copy file or directory", p);
					wrefresh(errwin);
					do { cmd = getch(); }
						while (cmd < 7 && 128 > cmd); // press any key
				} else {
					dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
					dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
				}
			}

			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		case KEY_F(6):
			draw_actwin2(actwin2, "Move", dirstate[active].items[dirstate[active].choice]->d_name,
				dirstate[active ^ 1].path);
			wrefresh(actwin2);
			do { cmd = getch(); }
				while (cmd != 10 && 27 != cmd); // enter or esc

			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(srcbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active ^ 1].path, p);

				result = 0;
				if (!(pid = fork()))
					return execl("/bin/mv", "mv", "-f", srcbuf, dstbuf, (char *)0);
				waitpid(pid, &result, 0);

				if (result < 0) {
					draw_errwin(errwin, "Couldn't move file or directory", p);
					wrefresh(errwin);
					do { cmd = getch(); }
						while (cmd < 7 && 128 > cmd); // press any key
				} else {
					if (dirstate[active].choice > 0) dirstate[active].choice--;
					dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
					dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
				}
			}
			
			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		case KEY_F(7):
			draw_actwin1(actwin1, "Create directory", "");
			wrefresh(actwin1);
			i = snprintf(dstbuf, MAX_STR-1, "%s/", dirstate[active].path);
			wattron(actwin1, COLOR_PAIR(1));

			s = p = dstbuf + i;
			getyx(actwin1, y, x);
			while (p - dstbuf < MAX_STR) {
				cmd = getchar();
				if (cmd == 13 || 27 == cmd) break;
				else if (cmd == KEY_BACKSPACE || cmd == 127) { // del
					if (p == s) continue;
					p--, x--;
					mvwaddch(actwin1, y, x, ' ');
					wmove(actwin1, y, x);
					wrefresh(actwin1);
				} else {
					wprintw(actwin1, "%c", cmd);
					*p = cmd, x++, p++;
					wrefresh(actwin1);
				}
			}
			*p = '\0';

			if (cmd != 27) {
				if (lstat(dstbuf, &st) == 0 || mkdir(dstbuf, 0700) == -1) {
					p = dstbuf + i;
					draw_errwin(errwin, "Couldn't create directory", p);
					wrefresh(errwin);
					do { cmd = getch(); }
						while (cmd < 7 && 128 > cmd); // press any key
				} else {
					dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
					dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
				}
			}

			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		case KEY_F(8):
			draw_actwin1(actwin1, "Delete", dirstate[active].items[dirstate[active].choice]->d_name);
			wrefresh(actwin1);
			do { cmd = getch(); }
				while (cmd != 10 && 27 != cmd); // enter or esc
			if (cmd != 27) {
				p = dirstate[active].items[dirstate[active].choice]->d_name;
				snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);

				result = 0;
				if (!(pid = fork()))
					return execl("/bin/rm", "rm", "-rf", dstbuf, (char *)0);
				waitpid(pid, &result, 0);

				if (result < 0) {
					draw_errwin(errwin, "Couldn't delete file or directory", p);
					wrefresh(errwin);
					do { cmd = getch(); }
						while (cmd < 7 && 128 > cmd); // press any key
				} else {
					if (dirstate[active].choice > 0) dirstate[active].choice--;
					dirstate[active].count = scandir(dirstate[active].path, &(dirstate[active].items), dot_filter, alphasort);
					dirstate[active ^ 1].count = scandir(dirstate[active ^ 1].path, &(dirstate[active ^ 1].items), dot_filter, alphasort);
				}
			}

			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		case KEY_F(10):
			exitflag = true;
			break;
		case KEY_STAB:
		case 9:
			director(dirwin[active], &dirstate[active], cmd, 0);
			wrefresh(dirwin[active]);
			active ^= 1;
			break;
		case KEY_ENTER:
		case 10:
			p = dirstate[active].items[dirstate[active].choice]->d_name;
			snprintf(dstbuf, MAX_STR-1, "%s/%s", dirstate[active].path, p);
			if (stat(dstbuf, &st) < 0) break;

			if (S_ISDIR(st.st_mode) && chdir(dstbuf) == 0) {
				if (lstat(dstbuf, &st) < 0) break;
				if (getcwd(dirstate[active].path, MAX_STR-1) == NULL) break;
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
				if (draw_shell(shell, dstbuf, p) < 0) {
					draw_errwin(errwin, "Couldn't exec", p);
					wrefresh(errwin);
				} else 	wrefresh(shell);

				do { cmd = getch(); }
					while (cmd < 7 && 128 > cmd); // press any key
				break;
			}

			director(dirwin[active ^ 1], &dirstate[active ^ 1], 0, 0);
			wrefresh(dirwin[active ^ 1]);
			break;
		default:
			director(dirwin[active], &dirstate[active], cmd, 1);
			wrefresh(dirwin[active]);
			break;
		}
	} while (!exitflag);

	free(dirstate[0].items);
	free(dirstate[1].items);
	delwin(dirwin[0]);
	delwin(dirwin[1]);
	delwin(status);
	delwin(menu);
	delwin(errwin);
	endwin();

	return 0;
}

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
	mvwprintw(help, 1, 1, "  ________  _             __");
	mvwprintw(help, 2, 1, " / ___/ _ \\(_)______ ____/ /____  ____");
	mvwprintw(help, 3, 1, "/ /__/ // / / __/ -_) __/ __/ _ \\/ __/");
	mvwprintw(help, 4, 1, "\\___/____/_/_/  \\__/\\__/\\__/\\___/_/");
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

int dot_filter(const struct dirent *ent) {
	return strcmp(ent->d_name, ".");
}

int wprintw_m(WINDOW *win, int attrs, char *path, char *name, int maxlen) {
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

	ret = wprintw(win, "%.*s", maxlen, name);
	if (attrs) wattroff(win, attrs);

	return ret;
}

void director(WINDOW *dir, wstate *s, int cmd, int active) {
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
				wprintw_m(dir, A_REVERSE, path, s->items[i]->d_name, s->width - 1);
			else wprintw_m(dir, 0, path, s->items[i]->d_name, s->width - 1);
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
