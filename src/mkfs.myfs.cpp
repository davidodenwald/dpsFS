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
#include <fstream>

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

    if (sizeof(basename(argv[1])) > NAME_LENGTH) {
        fprintf(stderr, "error: filename %s is too long\n", argv[1]);
        exit(ENAMETOOLONG);
    }

    BlockDevice blockDev = BlockDevice();
    blockDev.open(argv[1]);
    DMAP dmap = DMAP(&blockDev);
    dmap.create();
    FAT fat = FAT(&blockDev);
    RootDir rd = RootDir(&blockDev);

    for (int i = 2; i < argc; i++) {
        dpsFile file;
        char *filePath = argv[i];

        // save name of file
        if (sizeof(basename(filePath)) > NAME_LENGTH) {
            fprintf(stderr, "error: filename %s is too long\n", filePath);
            exit(ENAMETOOLONG);
        }
        strcpy(file.name, basename(filePath));

        // save stats of file
        if (stat(filePath, &file.stat) == -1) {
            fprintf(stderr, "error: file %s doesn't exist\n", filePath);
            exit(ENOENT);
        }

        // DMAP
        if (file.stat.st_size / 512 > FILES_SIZE) {
            fprintf(stderr, "error: file %s is to big for filesystem\n",
                    filePath);
            exit(EFBIG);
        }
        int blockCount = 0;
        if (file.stat.st_size % 512 == 0) {
            blockCount = file.stat.st_size / 512;
        } else {
            blockCount = file.stat.st_size / 512 + 1;
        }
        
        uint16_t *blocks = (uint16_t *)malloc(sizeof(uint16_t) * blockCount);
        dmap.getFree(blockCount, blocks);
        if (blocks[blockCount - 1] > FILES_INDEX + FILES_SIZE) {
            fprintf(stderr,
                    "error: not enough space for file %s in filesystem\n",
                    filePath);
            exit(EFBIG);
        }
        dmap.allocate(blockCount, blocks);
        file.firstBlock = blocks[0];

        // FAT
        int k;
        for (k = 1; k < blockCount; k++) {
            fat.write(blocks[k - 1], blocks[k]);
        }
        fat.write(blocks[k - 1], 0);

        // RootDir
        rd.write(i - 2, &file);

        // write Bytes
        std::ifstream fileStream(filePath);
        char buffer[BD_BLOCK_SIZE];
        for (int i = 0; i < blockCount; i++) {
            fileStream.read(buffer, BD_BLOCK_SIZE);
            blockDev.write(blocks[i], buffer);
        }
        fileStream.close();
    }
    
    blockDev.close();
    return 0;
}
