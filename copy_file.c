/*
 * copy_file.c
 *
 * Function description: Copy / move a file's preserving attributes.
 * Copyright (c) 2017 Aleksander Djuric. All rights reserved.
 * Distributed under the GNU Lesser General Public License (LGPL).
 * The complete text of the license can be found in the LICENSE
 * file included in the distribution.
 *
 */

#include "commander.h"
#include "copy_file.h"

int copy_data(int fdin, int fdout, void *buffer, size_t buffer_size, cp_callback cb_func, cp_state *cps) {
	struct pollfd ufdin, ufdout;
	int rsize, wsize, curpos;
	void *p;

	if (!buffer) return -1;

	ufdin.fd = fdin;
	ufdin.events = POLLIN | POLLRDNORM; // readable
	ufdout.fd = fdout;
	ufdout.events = POLLOUT | POLLWRNORM; // writable
	curpos = 1; // handle zero size files

	while ((rsize = read(fdin, buffer, buffer_size))) {
		if (rsize < 0) {
			if (errno == EAGAIN) {
				if (poll(&ufdin, 1, READ_TIMEOUT) < 0)
					return -1;
				continue;
			} else if (errno == EINTR) continue;
			return -1;
		}

		for (p = buffer; rsize > 0;) {
			wsize = write(fdout, buffer, rsize);
			if (wsize < 0) {
				if (errno == EAGAIN) {
					if (poll(&ufdout, 1, WRITE_TIMEOUT) < 0)
						return -1;
					continue;
				} else if (errno == EINTR) continue;
				return -1;
			}
			rsize -= wsize;
			p += wsize;
			curpos += wsize;
			cps->curpos = curpos;
			if (cb_func) cb_func(cps);
		}
	}

	cps->curpos = curpos;
	if (cb_func) cb_func(cps);
	return 0;
}

int copy_file(WINDOW *win, char *src, char *dst, int func, cp_callback cb_func) {
	char src_path[PATH_MAX];
	char dst_path[PATH_MAX];
	cp_state cps;
	struct stat ss, ds;
	void *buffer = NULL;
	char *p;
	size_t buffer_size;
	int fdin, fdout, rc;

	if (*src == '\0' || *dst == '\0') return 0;

	stat(src, &ss);
	cps.stat = win;
	cps.src = src;
	cps.dst = dst_path;
	cps.mode = ss.st_mode;
	cps.size = ss.st_size;
	cps.curpos = 0;
	cps.func = func;

	// get source file name
	p = src + strlen(src) - 1;
	while (p > src && *p == '/') p--;
	if ((p = strrchr(src, '/')) == 0) p = src;
		else p++;

	// build destination path
	if (stat(dst, &ds) == 0 && S_ISDIR(ds.st_mode))
		snprintf(dst_path, PATH_MAX, "%s/%s", dst, p);
	else strncpy(dst_path, dst, PATH_MAX-1);

	if (S_ISDIR(ss.st_mode)) {
		DIR *dir;

		if (mkdir(dst_path, cps.mode) < 0 && errno != EEXIST) return -1;
		if ((dir = opendir(src))) {
			struct dirent *d;

			while ((d = readdir(dir))) {
				if (d->d_name[0] == '.' &&
				(d->d_name[1] == '\0' || d->d_name[1] == '.')) continue;

				snprintf(src_path, PATH_MAX, "%s/%s", src, d->d_name);
				if (copy_file(win, src_path, dst_path, func, cb_func) < 0)
					return -1;
			}
		}
		closedir(dir);
		if (func == CP_MODE_MOVE) {
			rc = rmdir(src);
			cps.curpos = cps.size + 1;
			if (cb_func) cb_func(&cps);
			return rc;
		}

		return 0;
	}

	if (cb_func) cb_func(&cps);
	if (func == CP_MODE_MOVE) {
		rc = rename(src, dst_path);
		if (rc < 0 && errno != EXDEV) return rc;
		cps.curpos = cps.size + 1;
		if (cb_func) cb_func(&cps);
		return 0;
	}

	buffer_size = getpagesize();
	if (!(buffer = malloc(buffer_size))) return -1;

	if ((fdin = open(src, O_RDONLY)) < 0) return -1;
	if ((fdout = open(dst_path, O_WRONLY|O_CREAT|O_TRUNC, cps.mode & 0xfff)) < 0) {
		close(fdin);
		return -1;
	}

	rc = copy_data(fdin, fdout, buffer, buffer_size, cb_func, &cps);
	if (buffer) free(buffer);

	close(fdin);
	close(fdout);

	if (rc == 0 && func == CP_MODE_MOVE)
		return unlink(src);

	return rc;
}

int remove_file(WINDOW *win, char *path, int func, cp_callback cb_func) {
	char fname[PATH_MAX];
	cp_state cps;
	struct stat st;
	int rc;

	if (*path == '\0') return 0;

	stat(path, &st);
	cps.stat = win;
	cps.src = path;
	cps.dst = NULL;
	cps.mode = st.st_mode;
	cps.size = st.st_size;
	cps.curpos = 0;
	cps.func = func;

	if (S_ISDIR(st.st_mode)) {
		DIR *dir;

		if ((dir = opendir(path))) {
			struct dirent *d;

			while ((d = readdir(dir))) {
				if (d->d_name[0] == '.' &&
				(d->d_name[1] == '\0' || d->d_name[1] == '.')) continue;

				snprintf(fname, PATH_MAX, "%s/%s", path, d->d_name);
				if (remove_file(win, fname, func, cb_func) < 0)
					return -1;
			}
		}
		closedir(dir);
		if ((rc = rmdir(path)) == 0) {
			cps.curpos = cps.size + 1;
			if (cb_func) cb_func(&cps);
		}

		return rc;
	}

	if (cb_func) cb_func(&cps);
	if ((rc = unlink(path)) == 0) {
		cps.curpos = cps.size + 1;
		if (cb_func) cb_func(&cps);
	}

	return rc;
}
