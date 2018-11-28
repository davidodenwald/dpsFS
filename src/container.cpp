#include <string.h>

#include "container.h"

/**
 * Object representing the DMAP sector of the container.
 */
DMAP::DMAP(BlockDevice *blockDev) { this->blockDev = blockDev; }

DMAP::~DMAP() {}

/**
 * Initializes the DMAP sector.
 */
void DMAP::create() {
    char dmapBlock[BD_BLOCK_SIZE];
    memset(dmapBlock, 'F', BD_BLOCK_SIZE);

    for (unsigned short i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->write(i + DMAP_INDEX, dmapBlock);
    }
}

/**
 * Marks the given blocks as allocated.
 *
 * @param num   The number of blocks that must be written.
 * @param *arr  The array of block which must be written.
 */
void DMAP::allocate(unsigned short num, unsigned short *arr) {
    unsigned short currentBlock;
    int lastBlock = 0 + DMAP_INDEX;

    char dmapBlock[BD_BLOCK_SIZE];
    this->blockDev->read(lastBlock, dmapBlock);

    for (unsigned short i = 0; i < num; i++) {
        currentBlock = arr[i] / BD_BLOCK_SIZE + DMAP_INDEX;
        if (currentBlock != lastBlock) {
            printf("read and write\n");
            this->blockDev->write(lastBlock, dmapBlock);
            this->blockDev->read(currentBlock, dmapBlock);
            lastBlock = currentBlock;
        }
        printf("write pos %d in block %d.\n", arr[i] % BD_BLOCK_SIZE,
               currentBlock);
        dmapBlock[arr[i] % BD_BLOCK_SIZE] = 'A';
    }
    this->blockDev->write(lastBlock, dmapBlock);
}

/**
 * Gets free blocks.
 *
 * @param num   The number of blocks requested.
 * @param *arr  The array in which the blocks are stored.
 */
void DMAP::getFree(unsigned short num, unsigned short *arr) {
    char dmapBlock[BD_BLOCK_SIZE];
    for (unsigned short i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->read(i + DMAP_INDEX, dmapBlock);

        unsigned short found = 0;
        for (unsigned short k = 0; k < BD_BLOCK_SIZE; k++) {
            if (dmapBlock[k] == 'F') {
                if (found == num) {
                    return;
                }
                arr[found] = i * BD_BLOCK_SIZE + k;
                found++;
            }
        }
    }
}

/**
 * Object representing the RootDir sector of the container.
 */
RootDir::RootDir(BlockDevice *blockDev) { this->blockDev = blockDev; }

RootDir::~RootDir() {}

/**
 * Writes file information to the RootDir.
 * 
 * @param num       The number under which the file is stored.
 * @param *fileData The file information which must be stored.
 */
void RootDir::write(unsigned short num, dpsFile *fileData) {
    this->blockDev->write(ROOTDIR_INDEX + num, (char *)fileData);
}

/**
 * Reads file information from the RootDir.
 *
 * @param num       The number under which the file was stored.
 * @param *fileData The file information will be stored in here.
 */
void RootDir::read(unsigned short num, dpsFile *fileData) {
    this->blockDev->read(ROOTDIR_INDEX + num, (char *)fileData);
}