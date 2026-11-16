/* The GFOS port's start: the stack, the version string and PROGDIR:User
 * (the settings, the saved game and the high scores: User/data.db), made
 * before main() opens the database.
 * Copyright (c) 2026 AmiCradle Team. GPL 3 or later, like BreakHack. */
#include <sys/stat.h>

/* A Shell gives 4 KB of stack; SDL 2, Lua and SQLite want more. libnix's
 * startup sets this much up. */
unsigned long __stack = 512 * 1024;
static const char version[] __attribute__((used)) =
    "$VER: BreakHack 4.0.3 (16.11.2026)";

static void __attribute__((constructor)) amiga_user_drawer(void)
{
	mkdir("PROGDIR:User", 0755);
}
