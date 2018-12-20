#!/bin/bash

TESTFILE=mountdir/test

./mkfs.myfs container
./mount.myfs container logfile mountdir -s

touch $TESTFILE
if [ $? -eq 0 ]; then
    echo -e "\e[92mCreating a file was successful.\e[39m"
    stat $TESTFILE
else
    echo -e "\e[91mCreating a file failed.\e[39m"
fi

echo "Hello World" > $TESTFILE
if [ $? -eq 0 ]; then
    echo -e "\e[92mWriting to a file was successful.\e[39m"
    cat $TESTFILE
else
    echo -e "\e[91mWriting to a file failed.\e[39m"
fi

sleep 0.5
fusermount --unmount mountdir