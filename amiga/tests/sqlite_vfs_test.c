/* Host test for amiga/sqlite_vfs_stdio.c: SQLite built as the Amiga build
 * builds it (SQLITE_OS_OTHER, the stdio VFS), on a database written, closed,
 * opened again and read back, as BreakHack's settings, save and high scores
 * are. Copyright (c) 2026 AmiCradle Team. GPL 3 or later, like BreakHack. */
#include <stdio.h>
#include <string.h>
#include "sqlite3.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

static int count_rows(sqlite3 *db, const char *sql)
{
	sqlite3_stmt *st; int n = -1;
	if (sqlite3_prepare_v2(db, sql, -1, &st, NULL) == SQLITE_OK && sqlite3_step(st) == SQLITE_ROW)
		n = sqlite3_column_int(st, 0);
	sqlite3_finalize(st);
	return n;
}

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : "vfs-test.db";
	sqlite3 *db; sqlite3_stmt *st; char blob[3000]; int i;
	remove(path);
	CHECK(!strcmp(sqlite3_vfs_find(NULL)->zName, "stdio"));
	CHECK(sqlite3_open(path, &db) == SQLITE_OK);
	CHECK(sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS settings(key TEXT PRIMARY KEY, value INTEGER);"
	                       "CREATE TABLE IF NOT EXISTS hiscore(id INTEGER PRIMARY KEY, score REAL, lvl INTEGER);"
	                       "CREATE TABLE IF NOT EXISTS saves(id INTEGER PRIMARY KEY, data BLOB);", NULL, NULL, NULL) == SQLITE_OK);
	CHECK(sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) == SQLITE_OK);
	for (i = 0; i < 2000; i++) {
		char sql[128];
		sqlite3_snprintf(sizeof sql, sql, "INSERT INTO hiscore(score, lvl) VALUES(%d.5, %d)", i, i % 20);
		CHECK(sqlite3_exec(db, sql, NULL, NULL, NULL) == SQLITE_OK);
	}
	CHECK(sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) == SQLITE_OK);
	for (i = 0; i < (int) sizeof blob; i++) blob[i] = (char) (i * 7);
	CHECK(sqlite3_prepare_v2(db, "INSERT INTO saves(data) VALUES(?)", -1, &st, NULL) == SQLITE_OK);
	CHECK(sqlite3_bind_blob(st, 1, blob, sizeof blob, SQLITE_STATIC) == SQLITE_OK);
	CHECK(sqlite3_step(st) == SQLITE_DONE);
	sqlite3_finalize(st);
	CHECK(sqlite3_exec(db, "INSERT OR REPLACE INTO settings VALUES('music', 1)", NULL, NULL, NULL) == SQLITE_OK);
	/* a transaction rolled back leaves nothing behind */
	CHECK(sqlite3_exec(db, "BEGIN; DELETE FROM hiscore; ROLLBACK", NULL, NULL, NULL) == SQLITE_OK);
	CHECK(sqlite3_close(db) == SQLITE_OK);

	CHECK(sqlite3_open(path, &db) == SQLITE_OK);
	CHECK(count_rows(db, "SELECT count(*) FROM hiscore") == 2000);
	CHECK(count_rows(db, "SELECT lvl FROM hiscore WHERE score = 1999.5") == 19);
	CHECK(count_rows(db, "SELECT value FROM settings WHERE key = 'music'") == 1);
	CHECK(count_rows(db, "SELECT count(*) FROM saves") == 1);
	CHECK(sqlite3_prepare_v2(db, "SELECT data FROM saves", -1, &st, NULL) == SQLITE_OK);
	CHECK(sqlite3_step(st) == SQLITE_ROW);
	CHECK(sqlite3_column_bytes(st, 0) == (int) sizeof blob && !memcmp(sqlite3_column_blob(st, 0), blob, sizeof blob));
	sqlite3_finalize(st);
	CHECK(count_rows(db, "PRAGMA integrity_check") == -1 || 1);
	{
		const unsigned char *ok = NULL;
		CHECK(sqlite3_prepare_v2(db, "PRAGMA integrity_check", -1, &st, NULL) == SQLITE_OK);
		if (sqlite3_step(st) == SQLITE_ROW) ok = sqlite3_column_text(st, 0);
		CHECK(ok && !strcmp((const char *) ok, "ok"));
		sqlite3_finalize(st);
	}
	CHECK(sqlite3_close(db) == SQLITE_OK);
	remove(path);
	if (!fails) printf("SQLite on the stdio VFS: all checks passed\n");
	return fails != 0;
}
