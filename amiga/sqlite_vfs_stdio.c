/* SQLite's operating system layer for BreakHack on GFOS: one small VFS on
 * C's stdio, so the game's SQLite (lib/sqlite3, built SQLITE_OS_OTHER) needs
 * none of the POSIX calls libnix lacks (fcntl locks, pread, fsync on a
 * drawer). One program uses the database, so there are no locks.
 *
 * AmigaDOS refuses a seek past the end of a file: a write past the end
 * first fills the gap with zeros, and a read past the end reads zeros, as
 * SQLite asks (SQLITE_IOERR_SHORT_READ). A shrink (xTruncate) only happens
 * on VACUUM, which BreakHack never runs; SQLite trusts the page count in the
 * database header, so bytes left past it are harmless.
 *
 * tests/test_sqlite_vfs.sh builds it on the host with SQLite and checks a
 * database written, closed, opened again and read back.
 * Copyright (c) 2026 AmiCradle Team. GPL 3 or later, like BreakHack. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sqlite3.h"

#ifdef __amigaos__
#include <proto/dos.h>
#else
#include <unistd.h>
#endif

typedef struct {
	sqlite3_file base;
	FILE *f;
	char *deleteOnClose;
} StdioFile;

static sqlite3_int64 file_size(FILE *f)
{
	long here = ftell(f), end;
	if (fseek(f, 0, SEEK_END) != 0)
		return -1;
	end = ftell(f);
	fseek(f, here, SEEK_SET);
	return end;
}

static int x_close(sqlite3_file *pf)
{
	StdioFile *p = (StdioFile *) pf;
	int rc = fclose(p->f) == 0 ? SQLITE_OK : SQLITE_IOERR_CLOSE;
	if (p->deleteOnClose) {
		remove(p->deleteOnClose);
		sqlite3_free(p->deleteOnClose);
	}
	return rc;
}

static int x_read(sqlite3_file *pf, void *buf, int amt, sqlite3_int64 off)
{
	StdioFile *p = (StdioFile *) pf;
	sqlite3_int64 size = file_size(p->f);
	size_t got = 0;
	if (size < 0)
		return SQLITE_IOERR_READ;
	if (off < size) {
		if (fseek(p->f, (long) off, SEEK_SET) != 0)
			return SQLITE_IOERR_READ;
		got = fread(buf, 1, (size_t) amt, p->f);
		if (got < (size_t) amt && ferror(p->f)) {
			clearerr(p->f);
			return SQLITE_IOERR_READ;
		}
	}
	if (got < (size_t) amt) {
		memset((char *) buf + got, 0, (size_t) amt - got);
		return SQLITE_IOERR_SHORT_READ;
	}
	return SQLITE_OK;
}

static int x_write(sqlite3_file *pf, const void *buf, int amt, sqlite3_int64 off)
{
	StdioFile *p = (StdioFile *) pf;
	sqlite3_int64 size = file_size(p->f);
	if (size < 0)
		return SQLITE_IOERR_WRITE;
	if (off > size) {
		static const char zeros[512];
		if (fseek(p->f, 0, SEEK_END) != 0)
			return SQLITE_IOERR_WRITE;
		while (size < off) {
			size_t n = off - size > (sqlite3_int64) sizeof zeros ? sizeof zeros : (size_t) (off - size);
			if (fwrite(zeros, 1, n, p->f) != n)
				return SQLITE_IOERR_WRITE;
			size += n;
		}
	} else if (fseek(p->f, (long) off, SEEK_SET) != 0) {
		return SQLITE_IOERR_WRITE;
	}
	if (fwrite(buf, 1, (size_t) amt, p->f) != (size_t) amt)
		return SQLITE_IOERR_WRITE;
	return SQLITE_OK;
}

static int x_truncate(sqlite3_file *pf, sqlite3_int64 size)
{
	(void) pf; (void) size;
	return SQLITE_OK;
}

static int x_sync(sqlite3_file *pf, int flags)
{
	(void) flags;
	return fflush(((StdioFile *) pf)->f) == 0 ? SQLITE_OK : SQLITE_IOERR_FSYNC;
}

static int x_file_size(sqlite3_file *pf, sqlite3_int64 *size)
{
	*size = file_size(((StdioFile *) pf)->f);
	return *size < 0 ? SQLITE_IOERR_FSTAT : SQLITE_OK;
}

static int x_lock(sqlite3_file *pf, int level) { (void) pf; (void) level; return SQLITE_OK; }
static int x_check_reserved(sqlite3_file *pf, int *out) { (void) pf; *out = 0; return SQLITE_OK; }
static int x_file_control(sqlite3_file *pf, int op, void *arg) { (void) pf; (void) op; (void) arg; return SQLITE_NOTFOUND; }
static int x_sector_size(sqlite3_file *pf) { (void) pf; return 512; }
static int x_device_chars(sqlite3_file *pf) { (void) pf; return 0; }

static const sqlite3_io_methods io_methods = {
	1, x_close, x_read, x_write, x_truncate, x_sync, x_file_size,
	x_lock, x_lock, x_check_reserved, x_file_control, x_sector_size, x_device_chars
};

static int x_open(sqlite3_vfs *vfs, const char *name, sqlite3_file *pf, int flags, int *outFlags)
{
	StdioFile *p = (StdioFile *) pf;
	const char *mode;
	FILE *f;
	(void) vfs;
	memset(p, 0, sizeof *p);
	if (!name)    /* temporary files: SQLITE_TEMP_STORE=3 keeps them in memory */
		return SQLITE_CANTOPEN;
	if (flags & SQLITE_OPEN_READONLY) {
		mode = "rb";
	} else {
		mode = "r+b";
		if ((flags & SQLITE_OPEN_EXCLUSIVE) || (flags & SQLITE_OPEN_DELETEONCLOSE))
			mode = "w+b";
	}
	f = fopen(name, mode);
	if (!f && (flags & SQLITE_OPEN_CREATE))
		f = fopen(name, "w+b");
	if (!f)
		return SQLITE_CANTOPEN;
	p->f = f;
	if (flags & SQLITE_OPEN_DELETEONCLOSE)
		p->deleteOnClose = sqlite3_mprintf("%s", name);
	p->base.pMethods = &io_methods;
	if (outFlags)
		*outFlags = flags;
	return SQLITE_OK;
}

static int x_delete(sqlite3_vfs *vfs, const char *name, int syncDir)
{
	FILE *f;
	(void) vfs; (void) syncDir;
	if (!(f = fopen(name, "rb")))
		return SQLITE_IOERR_DELETE_NOENT;
	fclose(f);
	return remove(name) == 0 ? SQLITE_OK : SQLITE_IOERR_DELETE;
}

static int x_access(sqlite3_vfs *vfs, const char *name, int flags, int *out)
{
	FILE *f = fopen(name, flags == SQLITE_ACCESS_EXISTS || flags == SQLITE_ACCESS_READ ? "rb" : "r+b");
	(void) vfs;
	*out = f != NULL;
	if (f)
		fclose(f);
	return SQLITE_OK;
}

static int x_full_pathname(sqlite3_vfs *vfs, const char *name, int n, char *out)
{
	(void) vfs;
	sqlite3_snprintf(n, out, "%s", name);
	return SQLITE_OK;
}

static int x_randomness(sqlite3_vfs *vfs, int n, char *out)
{
	unsigned long x = (unsigned long) time(NULL) ^ (unsigned long) clock() ^ (unsigned long) (size_t) out;
	int i;
	(void) vfs;
	for (i = 0; i < n; i++) {
		x = x * 1103515245UL + 12345UL;
		out[i] = (char) (x >> 16);
	}
	return n;
}

static int x_sleep(sqlite3_vfs *vfs, int us)
{
	(void) vfs;
#ifdef __amigaos__
	Delay((us + 19999) / 20000);
#else
	usleep((useconds_t) us);
#endif
	return us;
}

static int x_current_time(sqlite3_vfs *vfs, double *out)
{
	(void) vfs;
	*out = 2440587.5 + (double) time(NULL) / 86400.0;
	return SQLITE_OK;
}

static int x_last_error(sqlite3_vfs *vfs, int n, char *out)
{
	(void) vfs; (void) n; (void) out;
	return 0;
}

static sqlite3_vfs stdio_vfs = {
	1, sizeof(StdioFile), 256, NULL, "stdio", NULL,
	x_open, x_delete, x_access, x_full_pathname,
	NULL, NULL, NULL, NULL,
	x_randomness, x_sleep, x_current_time, x_last_error
};

int sqlite3_os_init(void)
{
	return sqlite3_vfs_register(&stdio_vfs, 1);
}

int sqlite3_os_end(void)
{
	return SQLITE_OK;
}
