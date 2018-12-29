#!/bin/bash

TESTFILE=mountdir/test

./mkfs.myfs container
./mount.myfs container logfile mountdir -s

touch $TESTFILE
rm -rf $TESTFILE
if [ $? -eq 0 ]; then
    echo -e "\e[92mDeleting a file was successful.\e[39m"
else
    echo -e "\e[91mDeleting a file failed.\e[39m"
fi

ls $TESTFILE

sleep 0.5
fusermount --unmount mountdir