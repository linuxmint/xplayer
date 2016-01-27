#!/bin/sh

. `dirname $0`/mime-functions.sh

echo_mime () {
	echo "\"$i\","
}

SCHEMES=`grep -v '^#' $1`

echo "/* generated with uri-scheme-include.sh, don't edit */"
echo "static const gchar *uri_schemes[] = {"

for i in $SCHEMES ; do
	echo_mime;
done

echo "NULL"
echo "};"
