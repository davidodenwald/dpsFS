//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "blockdevice.h"
#include "container.h"
#include "myfs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "error: missing containerfile\n");
        fprintf(stderr, "usage: mkfs.myfs containerfile [input-file ...]\n");
        exit(ENOENT);
    }

    BlockDevice blockDev = BlockDevice();
    blockDev.open(argv[1]);

    DMAP dmap = DMAP(&blockDev);
    dmap.create();

    unsigned short arr[6];
    dmap.getFree(5, arr);

    
    for(int i = 0; i < 5; i++) {
        printf("%d\n", arr[i]);
    }
    arr[5] = 514;

    dmap.allocate(6, arr);
    

    /*
    for (int i = 2; i < argc; i++) {
        dpsFile file;
        char *filePath = argv[i];
        strcpy(file.name, basename(filePath));

        if (stat(filePath, &file.stat) == -1) {
            fprintf(stderr, "error: file %s doesn't exist\n", filePath);
            continue;
        }
        file.firstBlock = 1;
        RootDir rd = RootDir(&blockDev);
        rd.write(&file, 0);
    }
    */
    return 0;
}
