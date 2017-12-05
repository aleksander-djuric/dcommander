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
			cps->cp_cur = curpos;
			if (cb_func) cb_func(cps);
		}
	}

	cps->cp_cur = curpos;
	if (cb_func) cb_func(cps);
	return 0;
}

int copy_file(WINDOW *win, const char *src, const char *dst, int move_flag, cp_callback cb_func) {
	char src_path[PATH_MAX];
	char dst_path[PATH_MAX];
	cp_state cps;
	struct stat ss, ds;
	void *buffer = NULL;
	const char *p;
	mode_t mode;
	size_t size, buffer_size;
	int fdin, fdout, rc;

	if (*src == '\0' || *dst == '\0') return 0;

	stat(src, &ss);
	size = ss.st_size;
	mode = ss.st_mode;

	// get source file name
	p = src + strlen(src) - 1;
	while (p > src && *p == '/') p--;
	if ((p = strrchr(src, '/')) == 0) p = src;
		else p++;

	cps.stat = win;
	cps.src = src;
	cps.dst = dst_path;
	cps.move_flag = move_flag;
	cps.cp_top = size;
	cps.cp_cur = 0;
	if (cb_func) cb_func(&cps);

	// build destination path
	if (stat(dst, &ds) == 0 && S_ISDIR(ds.st_mode))
		snprintf(dst_path, PATH_MAX, "%s/%s", dst, p);
	else strncpy(dst_path, dst, PATH_MAX-1);

	if (S_ISDIR(ss.st_mode)) {
		DIR *src_dir;

		if (mkdir(dst_path, mode) < 0 && errno != EEXIST) return -1;
		if ((src_dir = opendir(src))) {
			struct dirent *dir;

			while ((dir = readdir(src_dir))) {
				if (dir->d_name[0] == '.' &&
				(dir->d_name[1] == '\0' || dir->d_name[1] == '.')) continue;

				snprintf(src_path, PATH_MAX, "%s/%s", src, dir->d_name);
				if (copy_file(win, src_path, dst_path, move_flag, cb_func) < 0)
					return -1;
			}
		}
		closedir(src_dir);
		if (move_flag)
			return rmdir(src);

		return 0;
	}

	if (move_flag) {
		rc = rename(src, dst_path);
		if (rc < 0 && errno != EXDEV) return rc;
		cps.cp_cur = size + 1; // handle zero size files
		if (cb_func) cb_func(&cps); // update status
		return 0;
	}

	if ((fdin = open(src, O_RDONLY)) < 0) return -1;
	if ((fdout = open(dst_path, O_WRONLY|O_CREAT|O_TRUNC, mode & 0xfff)) < 0) {
		close(fdin);
		return -1;
	}

	for (buffer_size = size;; buffer_size /= 2) {
		if (!(buffer = malloc(buffer_size)) &&
			buffer_size >= getpagesize()) continue;
		else break;
	}

	rc = copy_data(fdin, fdout, buffer, buffer_size, cb_func, &cps);
	if (buffer) free(buffer);

	close(fdin);
	close(fdout);

	if (rc == 0 && move_flag)
		return unlink(src);

	return rc;
}

