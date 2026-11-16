BreakHack for GFOS
==================

A port of BreakHack 4.0.3, Linus Probert's GPL dungeon crawler, from
https://github.com/liquidityc/breakhack at commit
b3a3fb33b291321b31d49dd20ee609f76af4cf37 (the last on SDL 2, before
upstream moved to SDL 3), for GFOS: AmigaOS 3.2 with GFOpenUp, on a 68040
or 68060 with an FPU, on GFOpenGPU's SDL 2.

Go down through ten dungeon levels, a room at a time: fight with the
mouse, the keys or skills, pick up artifacts, and keep your hiscore.

Status: written for Cradle's os32-gcc16-open6 stove; not yet run in a lab.


What you need
-------------

- GFOS: AmigaOS 3.2 with GFOpenUp and its GFOpenGPU part, which installs
  LIBS:GFOpenGPU/SDL2.module, SDL2_image.module and SDL2_mixer.module,
  LIBS:physfs.library and LIBS:z.library.
- A 68040 or 68060 with an FPU (not an LC040 or EC040), an RTG screen
  (the game's window is 672 x 576), about 16 MB of free memory and 18 MB
  on disk. It draws every frame with SDL's renderer: on a real 68040 it is
  slow; on AmiCradle it runs at speed.
- Nothing else: the tiles, music and sounds are in the drawer.


How to play
-----------

On GFOS: GFOpenUpTool INSTALL Parts/BreakHack puts the game in
SYS:Games/BreakHack. From the archive: run Install-BreakHack (or copy the
BreakHack drawer where you want it). Then double-click BreakHack. The
arrow keys (or hjkl) move and attack, 1 to 5 use the skills, the mouse
works the menus; the main menu's HOW TO PLAY shows the rest.

The settings, the saved game and the hiscores are kept in the drawer's
User drawer (User/data.db); the Installer keeps it on an update and
GFOpenUp never touches it.


Building
--------

    make -f amiga/Makefile.amigaos -j4 STOVE=/path/to/os32-gcc16-open6

or, in Cradle, stove_build with the os32-gcc16-open6 stove and target
package (package/build-amiga.sh), which also runs the host tests, makes
the GFOpenUp part (Parts/BreakHack) and packs BreakHack-VERSION-68040.lha.


What was changed
----------------

- src/main.c: on GFOS, PhysicsFS reads assets/ and data/ from the drawer
  as they are (no zip to inflate on a 68k), and the renderer is asked for
  render targets but not for acceleration.
- src/db.h: the database is PROGDIR:User/data.db, not .data.db in the
  current directory.
- amiga/sqlite_vfs_stdio.c: SQLite's operating system layer for libnix,
  on C's stdio (SQLITE_OS_OTHER), as libnix has no POSIX file locks or
  pread; amiga/tests/test_sqlite_vfs.sh builds it on the host with the
  same SQLite flags and checks a database written, reopened and read back.
- amiga/: the start (stack, $VER, the User drawer), config.h, the Makefile,
  the Installer spec (amicradle scripts/ports/port_installer.py) and the
  GlowIcons (scripts/ports/port_icons.py, from art/breakhack.svg).
- amiga/physfs/: physfs.library's header and the stub that opens it, from
  DalsinAI/gfopenphysfs d3a0ad78 (the stove's kit has no PhysicsFS yet);
  PhysicsFS itself runs in LIBS:physfs.library.
- package/, tools/ and tests/: the stove build, the GFOpenUp part (with its
  host test) and the archive.
- Lua 5.3.5 and SQLite 3.22 are upstream's own copies in lib/, built into
  the program; SDL and its satellites come from GFOpenGPU's kit, not from
  upstream's lib/ copies. Steam is left out, as upstream does without its SDK.


Licences
--------

BreakHack is GPL 3 (COPYING.txt). The tiles are DawnLike (DragonDePlatino,
palette by DawnBringer), CC-BY-SA 3.0; the music and most sounds are Eric
Matyas's (soundimage.org), free to use with credit; see Credits. The Amiga
files are The AmiCradle Team's, GPL 3 or later like the game (the packing
tools MIT).

Designed, tested and delivered by The AmiCradle Team.
