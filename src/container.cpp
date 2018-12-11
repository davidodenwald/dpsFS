#include <errno.h>
#include <string.h>

#include "container.h"
#include "myfs-structs.h"

/**
 * Checks if the address is a valid file-block address.
 */
bool checkBoundary(int address) {
    if (address < FILES_INDEX || address > FILES_SIZE) {
        return false;
    }
    return true;
}

/**
 * Object representing the Superblock sector of the container.
 */
Superblock::Superblock(BlockDevice *blockDev) { this->blockDev = blockDev; }

Superblock::~Superblock() {}

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

    for (uint16_t i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->write(i + DMAP_INDEX, dmapBlock);
    }
}

/**
 * Marks the given blocks as allocated.
 *
 * @param num   The number of blocks that must be written.
 * @param *arr  The array of block which must be written.
 */
void DMAP::allocate(uint16_t num, uint16_t *arr) {
    uint16_t currentBlock;
    int lastBlock = 0 + DMAP_INDEX;

    char dmapBlock[BD_BLOCK_SIZE];
    this->blockDev->read(lastBlock, dmapBlock);

    for (uint16_t i = 0; i < num; i++) {
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
void DMAP::getFree(uint16_t num, uint16_t *arr) {
    char dmapBlock[BD_BLOCK_SIZE];
    uint16_t found = 0;

    for (uint16_t i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->read(i + DMAP_INDEX, dmapBlock);

        for (uint16_t k = 0; k < BD_BLOCK_SIZE; k++) {
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
 *
 * @return          EMFILE when num was bigger than NUM_DIR_ENTRIES or negativ;
 *                  0 otherwise.
 */
int RootDir::write(uint16_t num, dpsFile *fileData) {
    if (num > NUM_DIR_ENTRIES) {
        return EMFILE;
    }
    this->blockDev->write(ROOTDIR_INDEX + num, (char *)fileData);
    return 0;
}

/**
 * Reads file information from the RootDir.
 *
 * @param num       The number under which the file was stored.
 * @param *fileData The file information will be stored in here.
 *
 * @return          EMFILE when num was bigger than NUM_DIR_ENTRIES or negativ;
 *                  0 otherwise.
 */
int RootDir::read(uint16_t num, dpsFile *fileData) {
    if (num > NUM_DIR_ENTRIES || num < 1) {
        return EMFILE;
    }
    this->blockDev->read(ROOTDIR_INDEX + num, (char *)fileData);
    return 0;
}

/**
 * Gets the fileinformation by filename.
 *
 * @param *name     The name of the file.
 * @param *fileData The file information will be stored in here.
 *
 * @return          ENOENT when no file with name was found; 0 otherwise.
 */
int RootDir::get(char *name, dpsFile *fileData) {
    for (uint16_t i = ROOTDIR_INDEX; i < (ROOTDIR_INDEX + ROOTDIR_SIZE); i++) {
        this->blockDev->read(i, (char *)fileData);
        if (strcmp(fileData->name, name) == 0) {
            return 0;
        }
    }
    return ENOENT;
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
void FAT::write(uint16_t curAddress, uint16_t nextAddress) {
    if (!checkBoundary(curAddress) ||
        (!checkBoundary(nextAddress) && nextAddress != 0) ||
        curAddress == nextAddress) {
        return;
    }

    uint16_t blockAddr = (curAddress - FILES_INDEX) / 256 + FAT_INDEX;
    uint16_t charAddr = ((curAddress - FILES_INDEX) * 2) % 256;
    char fatBlock[BD_BLOCK_SIZE];

    this->blockDev->read(blockAddr, fatBlock);
    fatBlock[charAddr] = (nextAddress >> 8);
    fatBlock[charAddr + 1] = (nextAddress & 0xff);
    this->blockDev->write(blockAddr, fatBlock);
}

/**
 * Reads next address from the given address.
 *
 */
uint16_t FAT::read(uint16_t curAddress) {
    if (!checkBoundary(curAddress)) {
        return 0;
    }
    uint16_t blockAddr = (curAddress - FILES_INDEX) / 256 + FAT_INDEX;
    uint16_t charAddr = ((curAddress - FILES_INDEX) * 2) % 256;
    char fatBlock[BD_BLOCK_SIZE];

    blockDev->read(blockAddr, fatBlock);
    uint8_t first = (uint16_t)fatBlock[charAddr];
    uint8_t second = (uint16_t)fatBlock[charAddr + 1];
    uint16_t combined = (first << 8) | second;
    return combined;
}