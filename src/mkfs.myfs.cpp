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

    if (argc - 2 > NUM_DIR_ENTRIES) {
        fprintf(stderr, "error: only %d files are allowed\n", NUM_DIR_ENTRIES);
        exit(1);
    }

    BlockDevice blockDev = BlockDevice();
    blockDev.open(argv[1]);
    Superblock sb = Superblock(&blockDev);
    DMAP dmap = DMAP(&blockDev);
    dmap.create();
    FAT fat = FAT(&blockDev);
    RootDir rd = RootDir(&blockDev, 0);

    for (int i = 2; i < argc; i++) {
        dpsFile *file = (dpsFile*) malloc(BD_BLOCK_SIZE);
        char *filePath = argv[i];

        // save name of file
        if (sizeof(basename(filePath)) > NAME_LENGTH) {
            fprintf(stderr, "error: filename %s is too long\n", filePath);
            exit(ENAMETOOLONG);
        }
        strcpy(file->name, basename(filePath));

        // check if file already exists
        if (rd.exists(file->name) == 0) {
            fprintf(stderr, "error: filename %s is a duplicate\n", file->name);
            exit(EEXIST);
        }

        // save stats of file
        if (stat(filePath, &file->stat) == -1) {
            fprintf(stderr, "error: file %s doesn't exist\n", filePath);
            exit(ENOENT);
        }

        // DMAP
        if (file->stat.st_size / BD_BLOCK_SIZE > FILES_SIZE) {
            fprintf(stderr, "error: file %s is to big for filesystem\n",
                    filePath);
            exit(EFBIG);
        }
        int blockCount = 0;
        if (file->stat.st_size % BD_BLOCK_SIZE == 0) {
            blockCount = file->stat.st_size / BD_BLOCK_SIZE;
        } else {
            blockCount = file->stat.st_size / BD_BLOCK_SIZE + 1;
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
        file->firstBlock = blocks[0];

        // FAT
        int k;
        for (k = 1; k < blockCount; k++) {
            fat.write(blocks[k - 1], blocks[k]);
        }
        fat.write(blocks[k - 1], 0);

        // RootDir
        rd.write(i - 2, file);
        free(file);

        // write Bytes
        std::ifstream fileStream(filePath);
        char buffer[BD_BLOCK_SIZE];
        for (int i = 0; i < blockCount; i++) {
            // set all array items to 0
            memset(buffer, 0, sizeof(buffer));
            fileStream.read(buffer, BD_BLOCK_SIZE);
            blockDev.write(blocks[i], buffer);
        }
        fileStream.close();
        free(blocks);
    }

    // Superblock
    sbStats *s = (sbStats*) malloc(BD_BLOCK_SIZE);
    s->fileCount = 2;
    sb.write(s);
    free(s);

    // Test
    sbStats *tmpSbStats = (sbStats*) malloc(BD_BLOCK_SIZE);
    sb.read(tmpSbStats);
    printf("File Count: %d\n", tmpSbStats->fileCount);
    free(tmpSbStats);

    for (int i = 0; i < rd.len(); i++) {
        dpsFile tmpDpsFile;
        rd.read(i, &tmpDpsFile);

        uint16_t block;
        char *buffer = (char *) malloc(BD_BLOCK_SIZE);
        for(block = tmpDpsFile.firstBlock; block != 0; block = fat.read(block)) {
            blockDev.read(block, buffer);
            printf("%s", buffer);
        }
        free(buffer);
    }

    blockDev.close();
    return 0;
}
