#!/bin/sh

MAC_PREFIX="aa:bb:cc"

set_mac() {
    M4=`od -An -N2 -i /dev/random | sed -e 's/ //g' | \
                                awk '{ print $1 % 256 }'`
    M5=`od -An -N2 -i /dev/random | sed -e 's/ //g' | \
                                awk '{ print $1 % 256 }'`
    MAC=`printf ${MAC_PREFIX}:%02x:%02x:%02x ${M4} ${M5} ${IF}`
    ifconfig ${EIFACE} link $MAC
}

ngctl mkpeer eiface ether ether
EIFACE=`ngctl l | grep ngeth | tail -n 1| awk '{print $2}'`
IF="0"

set_mac
ifconfig ${EIFACE} inet 192.168.112.1/24 up

ngctl mkpeer ${EIFACE}: bridge  ether link0
ngctl name ${EIFACE}:ether bridge0

for IF in 1 2 3 4 5 6 7 8 9; do
    ngctl mkpeer eiface ether ether
    EIFACE=`ngctl l | grep ngeth | tail -n 1| awk '{print $2}'`
    set_mac

    ngctl connect ${EIFACE}: bridge0: ether link${IF}
    ifconfig ${EIFACE} up
done

