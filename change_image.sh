#!/bin/sh

if [ -z "${1}" ]
then
	echo "ERROR: I need exactly one argument."
	exit 1
fi

TEST=test.gif
[ -L ${TEST} ] && rm ${TEST}
ln -s gifs/animated${1}.gif ${TEST}

exit 0
