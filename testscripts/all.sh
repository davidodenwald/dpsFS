#!/bin/bash

make all
make unittest

./unittest

if [ ! -d "mountdir" ]; then
    mkdir mountdir
fi
fusermount --unmount mountdir > /dev/null 2>&1

./testscripts/read.sh
./testscripts/write.sh
./testscripts/delete.sh