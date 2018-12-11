//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <errno.h>
#include <stdio.h>
#include <string.h>
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

    FAT fat = FAT(&blockDev);

    RootDir rd = RootDir(&blockDev);

    for (uint16_t i = 2; i < argc; i++) {
        dpsFile file;
        char *filePath = argv[i];
        strcpy(file.name, basename(filePath));

        if (stat(filePath, &file.stat) == -1) {
            fprintf(stderr, "error: file %s doesn't exist\n", filePath);
            continue;
        }

        // DMAP
        unsigned long blockCount = 0;
        if (file.stat.st_size % 512 == 0) {
            blockCount = file.stat.st_size / 512;
        } else {
            blockCount = file.stat.st_size / 512 + 1;
        }
        if (blockCount > FILES_SIZE) {
            fprintf(stderr, "error: file %s is to big for filesystem\n",
                    filePath);
            continue;
        }

        uint16_t *blocks = (uint16_t *)malloc(sizeof(uint16_t) * blockCount);
        dmap.getFree(blockCount, blocks);
        dmap.allocate(blockCount, blocks);
        file.firstBlock = blocks[0];

        // FAT
        uint16_t k;
        for (k = 1; k < blockCount; k++) {
            printf("Write: %d -> %d\n", blocks[k - 1], blocks[k]);
            fat.write(blocks[k - 1], blocks[k]);
        }
        printf("Write: %d -> %d\n", blocks[k - 1], 0);
        fat.write(blocks[k - 1], 0);

        // RootDir
        rd.write(i - 2, &file);
    }

    blockDev.close();
    return 0;
}
