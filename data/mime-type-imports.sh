#!/bin/sh

. `dirname $0`/mime-functions.sh

echo_mime () {
	echo "    \"$i\","
}


echo "/* generated with mime-type-imports.sh in the totem module, don't edit or"
echo "   commit in the sushi module without filing a bug against totem */"

echo "let audioTypes = ["
get_audio_mimetypes $1;
for i in $MIMETYPES ; do
	echo_mime;
done
echo -e "];\n"
echo "let videoTypes = ["
get_video_mimetypes $1;
for i in $MIMETYPES ; do
	echo_mime;
done
echo "];"

