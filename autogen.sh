#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ACLOCAL_FLAGS="-I libgd $ACLOCAL_FLAGS"

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level xplayer directory"
    exit 1
}

mkdir -p m4

gtkdocize --copy || exit 1
intltoolize --force --copy --automake || exit 1
autoreconf --verbose --force --install || exit 1

cd "$olddir"
if [ "$NOCONFIGURE" = "" ]; then
    "$srcdir/configure" "$@" || exit 1

    if [ "$1" = "--help" ]; then exit 0 else
        echo "Now type 'make' to compile" || exit 1
    fi
else
    echo "Skipping configure process."
fi
