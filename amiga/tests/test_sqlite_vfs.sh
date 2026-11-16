#!/bin/sh
# Builds lib/sqlite3 with amiga/sqlite_vfs_stdio.c on the host, with the
# Amiga build's SQLite flags (amiga/Makefile.amigaos SQLITE_DEFS), and runs
# amiga/tests/sqlite_vfs_test.c. MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
DEFS=$(sed -n 's/^SQLITE_DEFS *= *//p' "$ROOT/amiga/Makefile.amigaos")
CC=${HOSTCC:-cc}
$CC -O1 -w $DEFS -I"$ROOT/lib/sqlite3" -o "$T/vfs-test" \
    "$ROOT/lib/sqlite3/sqlite3.c" "$ROOT/amiga/sqlite_vfs_stdio.c" "$HERE/sqlite_vfs_test.c"
"$T/vfs-test" "$T/test.db"
