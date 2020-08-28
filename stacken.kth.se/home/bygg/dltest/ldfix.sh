#!/bin/sh

UNAME=`uname`

case ${UNAME} in
    Linux)
        echo "-ldl"
	exit 0 ;;
esac

echo ""
