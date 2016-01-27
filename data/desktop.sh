#!/bin/sh

echo_mime () {
	printf "$i;";
}

echo_handler () {
	printf "x-scheme-handler/$i;";
}

MIMETYPES=`grep -v ^# $1`
printf MimeType=;
for i in $MIMETYPES ; do
	echo_mime;
done

MIMETYPES=`grep -v ^# $2`
for i in $MIMETYPES ; do
	echo_handler;
done

echo ""
