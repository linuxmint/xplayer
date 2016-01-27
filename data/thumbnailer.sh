#!/bin/sh

. `dirname $0`/mime-functions.sh

echo_mime () {
	printf "$i;";
}

printf MimeType=;

get_video_mimetypes $1;
for i in $MIMETYPES ; do
	echo_mime;
done

get_audio_mimetypes $1;
for i in $MIMETYPES ; do
	echo_mime;
done

echo ""
