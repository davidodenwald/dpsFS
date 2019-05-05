#!/bin/bash

TESTFILE=mountdir/test

./mkfs.myfs container
./mount.myfs container logfile mountdir -s

for i in {1..10}; do
    cat big.txt > $TESTFILE
    if [ $? -eq 0 ]; then
        echo -e "\e[92mCreating a file was successful.\e[39m"
        ls -alh $TESTFILE
    else
        echo -e "\e[91mCreating a file failed.\e[39m"
    fi

    echo '' > $TESTFILE
done

sleep 0.5
fusermount --unmount mountdir