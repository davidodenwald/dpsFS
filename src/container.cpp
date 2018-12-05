#include <string.h>

#include "container.h"


/**
 * 
 */
bool checkBoundary(int address) {
     if (address < FILES_INDEX || address > FILES_SIZE) {
        return false;
    }
    return true;
}

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
        currentBlock = (arr[i] - FILES_INDEX) / BD_BLOCK_SIZE + DMAP_INDEX;
        if (currentBlock != lastBlock) {
            this->blockDev->write(lastBlock, dmapBlock);
            this->blockDev->read(currentBlock, dmapBlock);
            lastBlock = currentBlock;
        }
        dmapBlock[(arr[i] - FILES_INDEX) % BD_BLOCK_SIZE] = 'A';
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
    unsigned short found = 0;

    for (unsigned short i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->read(i + DMAP_INDEX, dmapBlock);

        for (unsigned short k = 0; k < BD_BLOCK_SIZE; k++) {
            if (dmapBlock[k] == 'F') {
                if (found == num) {
                    return;
                }
                arr[found] = (i * BD_BLOCK_SIZE + k) + FILES_INDEX;
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

/**
 * Object representing the FAT sector of the container.
 */
FAT::FAT(BlockDevice *blockDev) { this->blockDev = blockDev; }

FAT::~FAT() {}

/**
 * Writes next address to the given address.
 *
 */
void FAT::write(unsigned short curAddress, unsigned short nextAddress) {
    if (!checkBoundary(curAddress) || (!checkBoundary(nextAddress) && nextAddress != 0) || curAddress == nextAddress) {
        return;
    }
    char fatBlock[BD_BLOCK_SIZE];
    this->blockDev->read(FAT_INDEX + curAddress / 256, fatBlock);
    fatBlock[curAddress % 256] = (nextAddress >> 8);
    fatBlock[curAddress % 256 + 1] = (nextAddress & 0xff);
    this->blockDev->write(FAT_INDEX + curAddress / 256, fatBlock);
}

/**
 * Reads next address from the given address.
 *
 */
unsigned short FAT::read(unsigned short curAddress) {
    if (!checkBoundary(curAddress)) {
        return -1;
    }

    char fatBlock[BD_BLOCK_SIZE];
    blockDev->read(FAT_INDEX + curAddress / 256, fatBlock);
    return ((((unsigned short)fatBlock[curAddress % 256]) << 8) +
            (unsigned short)fatBlock[curAddress % 256 + 1]);
}