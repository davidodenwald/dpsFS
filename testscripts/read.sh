#!/bin/bash

MOUNTDIR=mountdir
TESTFILE=big.txt

./mkfs.myfs container $TESTFILE
./mount.myfs container logfile $MOUNTDIR -s

diff $TESTFILE $MOUNTDIR/$TESTFILE
if [ $? -eq 0 ]; then
    echo -e "\e[92mReading a file was successful.\e[39m"
else
    echo -e "\e[91mReading a file failed.\e[39m"
fi

sleep 0.5
fusermount --unmount mountdir