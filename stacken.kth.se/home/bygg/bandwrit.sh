#! /bin/bash
[ x$1 = x ] && echo 'No tape name given' && exit 1

mt -f /dev/nrst8 stat

echo -n 'confirm>' ; read

/usr/gnu/bin/dd if=${1}.img bs=2720c of=/dev/rst8
