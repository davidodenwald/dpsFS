//
//  mk.myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 07.09.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#ifdef __APPLE__
#include <libgen.h>
#endif

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

    // clear container file
    fclose(fopen(argv[1], "w"));

    if (argc - 2 > NUM_DIR_ENTRIES) {
        fprintf(stderr, "error: only %d files are allowed\n", NUM_DIR_ENTRIES);
        exit(1);
    }

    BlockDevice blockDev = BlockDevice();
    blockDev.open(argv[1]);
    DMAP dmap = DMAP(&blockDev);
    dmap.create();
    FAT fat = FAT(&blockDev);
    RootDir rd = RootDir(&blockDev);

    for (int i = 2; i < argc; i++) {
        dpsFile *file = (dpsFile *)malloc(BD_BLOCK_SIZE);
        char *filePath = argv[i];

        // save name of file
        if (strlen(basename(filePath)) > NAME_LENGTH) {
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

        file->stat.st_atime = time(NULL);
        file->stat.st_ctime = time(NULL);
        file->stat.st_uid = getgid();
        file->stat.st_gid = getuid();
        file->stat.st_mode = S_IFREG | 0444;

        // correct file blocksize and blocks
        file->stat.st_blksize = 512;
        if (file->stat.st_size % BD_BLOCK_SIZE == 0) {
            file->stat.st_blocks = file->stat.st_size / BD_BLOCK_SIZE;
        } else {
            file->stat.st_blocks = file->stat.st_size / BD_BLOCK_SIZE + 1;
        }

        if (file->stat.st_blocks == 0) {
            file->firstBlock = 0;
        } else {
            // DMAP
            uint16_t *blocks =
                (uint16_t *)malloc(sizeof(uint16_t) * file->stat.st_blocks);
            if (dmap.getFree(file->stat.st_blocks, blocks) != 0) {
                fprintf(stderr,
                        "error: not enough space for file %s in filesystem\n",
                        filePath);
            }
            dmap.allocate(file->stat.st_blocks, blocks);
            file->firstBlock = blocks[0];

            // FAT
            int k = 1;
            for (; k < file->stat.st_blocks; k++) {
                fat.write(blocks[k - 1], blocks[k]);
            }
            fat.write(blocks[k - 1], 0);

            // write Bytes
            std::ifstream fileStream(filePath);
            char buffer[BD_BLOCK_SIZE];
            for (int i = 0; i < file->stat.st_blocks; i++) {
                // set all array items to 0
                memset(buffer, 0, sizeof(buffer));
                fileStream.read(buffer, BD_BLOCK_SIZE);
                blockDev.write(blocks[i], buffer);
            }
            fileStream.close();
            free(blocks);
        }
        // RootDir
        rd.write(file);
        free(file);
    }

    dmap.toFile();
    fat.toFile();
    rd.toFile();
    blockDev.close();
    return 0;
}
