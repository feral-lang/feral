#include "FS.hpp"

#include <cstdlib>
#include <string>
#include <vector>

#include "Env.hpp"

#if defined(OS_WINDOWS)
#include <direct.h>
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace fer
{
namespace fs
{

int total_lines = 0;

bool exists(const char *loc)
{
#if defined(OS_WINDOWS)
	return GetFileAttributes(loc) != INVALID_FILE_ATTRIBUTES &&
	       GetLastError() != ERROR_FILE_NOT_FOUND;
#else
	return access(loc, F_OK) != -1;
#endif
}

bool read(const char *file, String &data)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(file, "r");
	if(fp == NULL) {
		std::cerr << "Error: failed to open source file: " << file << "\n";
		return false;
	}

	while((read = getline(&line, &len, fp)) != -1) data += line;

	fclose(fp);
	if(line) free(line);

	if(data.empty()) {
		std::cerr << "Error: encountered empty file: " << file << "\n";
		return false;
	}

	return true;
}

String absPath(const char *loc)
{
	static char abs[MAX_PATH_CHARS];
#if defined(OS_WINDOWS)
	_fullpath(abs, loc, MAX_PATH_CHARS);
#else
	realpath(loc, abs);
#endif
	return abs;
}

bool setCWD(const char *path)
{
#if defined(OS_WINDOWS)
	return _chdir(path) != 0;
#else
	return chdir(path) != 0;
#endif
}

String getCWD()
{
	static char cwd[MAX_PATH_CHARS];
	if(getcwd(cwd, sizeof(cwd)) != NULL) {
		return cwd;
	}
	return "";
}

String home()
{
	static String _home = env::get("HOME");
	return _home;
}

} // namespace fs
} // namespace fer

#if defined(OS_WINDOWS)
// getdelim and getline functions from NetBSD libnbcompat:
// https://github.com/archiecobbs/libnbcompat

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
{
	char *ptr, *eptr;

	if(*buf == NULL || *bufsiz == 0) {
		*bufsiz = BUFSIZ;
		if((*buf = (char *)malloc(*bufsiz)) == NULL) return -1;
	}

	for(ptr = *buf, eptr = *buf + *bufsiz;;) {
		int c = fgetc(fp);
		if(c == -1) {
			if(feof(fp)) {
				ssize_t diff = (ssize_t)(ptr - *buf);
				if(diff != 0) {
					*ptr = '\0';
					return diff;
				}
			}
			return -1;
		}
		*ptr++ = c;
		if(c == delimiter) {
			*ptr = '\0';
			return ptr - *buf;
		}
		if(ptr + 2 >= eptr) {
			char *nbuf;
			size_t nbufsiz = *bufsiz * 2;
			ssize_t d      = ptr - *buf;
			if((nbuf = (char *)realloc(*buf, nbufsiz)) == NULL) return -1;
			*buf	= nbuf;
			*bufsiz = nbufsiz;
			eptr	= nbuf + nbufsiz;
			ptr	= nbuf + d;
		}
	}
	return 0;
}

ssize_t getline(char **buf, size_t *bufsiz, FILE *fp) { return getdelim(buf, bufsiz, '\n', fp); }
#endif