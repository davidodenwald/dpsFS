#!/bin/bash

make all
make unittest

./unittest

./testscripts/read.sh
./testscripts/write.sh
./testscripts/delete.sh