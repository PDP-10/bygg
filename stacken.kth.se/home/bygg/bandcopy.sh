#!/bin/bash -x
[ x$1 = x ] && echo 'No tape name given' && exit 1

mt -f /dev/nrst0 rew
mt -f /dev/nrst0 stat
#mt -f /dev/nrmt0 stat
echo -n 'confirm>' ; read
/usr/gnu/bin/dd if=/dev/rst0 bs=2720c of=${1}.img  #### | /usr/gnu/bin/dd of=/dev/rmt0 bs=2720c 
~bygg/backup -tvf ${1}.img > ${1}.dir &

