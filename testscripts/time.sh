#!/bin/bash

./mkfs.myfs container
./mount.myfs container logfile mountdir -s


cp big.txt mountdir/test1
if [ $? -eq 0 ]; then
    echo -e "\e[92mAppending to a file was successful.\e[39m"
else
    echo -e "\e[91mAppending to a file failed.\e[39m"
fi

cp big.txt mountdir/test2
if [ $? -eq 0 ]; then
    echo -e "\e[92mAppending to a file was successful.\e[39m"
else
    echo -e "\e[91mAppending to a file failed.\e[39m"
fi

cp big.txt mountdir/test3
if [ $? -eq 0 ]; then
    echo -e "\e[92mAppending to a file was successful.\e[39m"
else
    echo -e "\e[91mAppending to a file failed.\e[39m"
fi

cp big.txt mountdir/test4
if [ $? -eq 0 ]; then
    echo -e "\e[92mAppending to a file was successful.\e[39m"
else
    echo -e "\e[91mAppending to a file failed.\e[39m"
fi

cp big.txt mountdir/test5
if [ $? -eq 0 ]; then
    echo -e "\e[92mAppending to a file was successful.\e[39m"
else
    echo -e "\e[91mAppending to a file failed.\e[39m"
fi

ls -al mountdir

sleep 0.5
fusermount --unmount mountdir

# Nothing in memory
# 123,478 s

# FAT and DMAP in memory (toFile in write)
# 10.194 s

# FAT and DMAP in memory (toFile in release)
# 2.915 s