#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "container.h"
#include "myfs-structs.h"

// #define DEBUG 1

/**
 * Object representing the Superblock sector of the container.
 */
Superblock::Superblock(BlockDevice *blockDev) { this->blockDev = blockDev; }

Superblock::~Superblock() {}

/**
 * Reads the sbStats struct from the superblocck.
 *
 * @param *content  the struct in which the sbStats will be saved.
 */
int Superblock::read(sbStats *content) {
#ifdef DEBUG
    fprintf(stderr, "Reading SuperBlock\n");
#endif
    if (this->blockDev->read(0, (char *)content) != 0) {
        return EIO;
    }
    return 0;
}

/**
 * Writes the sbStats struct to the superblocck.
 *
 * @param *content  the sbStats struct which should be written.
 */
int Superblock::write(sbStats *content) {
#ifdef DEBUG
    fprintf(stderr, "Writing SuperBlock\n");
#endif
    if (this->blockDev->write(0, (char *)content) != 0) {
        return EIO;
    }
    return 0;
}

/**
 * Object representing the DMAP sector of the container.
 */
DMAP::DMAP(BlockDevice *blockDev) { this->blockDev = blockDev; }

DMAP::~DMAP() {}

/**
 * Initializes the DMAP sector.
 * @return  0 when successful.
 *          EIO when write failed.
 */
int DMAP::create() {
#ifdef DEBUG
    fprintf(stderr, "Creating DMAP\n");
#endif
    char dmapBlock[BD_BLOCK_SIZE];
    memset(dmapBlock, 'F', BD_BLOCK_SIZE);

    for (int i = 0; i < DMAP_SIZE - 1; i++) {
        if (this->blockDev->write(i + DMAP_INDEX, dmapBlock) != 0) {
            return EIO;
        }
    }
    // last block only represents 63 blocks - rest is filled with 0 bytes.
    memset(dmapBlock, '0', BD_BLOCK_SIZE);
    memset(dmapBlock, 'F', FILES_SIZE % BD_BLOCK_SIZE);
    if (this->blockDev->write(DMAP_INDEX + DMAP_SIZE-1, dmapBlock) != 0) {
            return EIO;
    }
    return 0;
}

/**
 * Gets free blocks.
 *
 * @param num   The number of blocks requested.
 * @param *arr  The array in which the blocks are stored.
 *
 * @return      the number of blocks returned.
 *              -EIO when read failed.
 */
int DMAP::getFree(uint16_t num, uint16_t *arr) {
#ifdef DEBUG
    fprintf(stderr, "Get Free %d blocks from DMAP\n", num);
#endif
    char dmapBlock[BD_BLOCK_SIZE];
    uint16_t found = 0;

    for (uint16_t i = 0; i < DMAP_SIZE; i++) {
        if (this->blockDev->read(i + DMAP_INDEX, dmapBlock) != 0) {
            return -EIO;
        }

        for (uint16_t k = 0; k < BD_BLOCK_SIZE; k++) {
            if (dmapBlock[k] == 'F') {
                if (found == num) {
                    return found;
                }
                arr[found] = (i * BD_BLOCK_SIZE + k) + FILES_INDEX;
                found++;
            }
        }
    }
    return found;
}

/**
 * Marks the given blocks as allocated.
 *
 * @param num   The number of blocks that must be written.
 * @param *arr  The array of block which must be written.
 *
 *  @return     0 when successful.
 *              EIO when read failed.
 */
int DMAP::allocate(uint16_t num, uint16_t *arr) {
#ifdef DEBUG
    fprintf(stderr, "Allocate %d blocks in DMAP\n", num);
#endif
    uint16_t currentBlock;
    int lastBlock = 0 + DMAP_INDEX;

    char dmapBlock[BD_BLOCK_SIZE];
    if (this->blockDev->read(lastBlock, dmapBlock) != 0) {
        return EIO;
    }

    for (uint16_t i = 0; i < num; i++) {
        currentBlock = (arr[i] - FILES_INDEX) / BD_BLOCK_SIZE + DMAP_INDEX;
        if (currentBlock != lastBlock) {
            if (this->blockDev->write(lastBlock, dmapBlock) != 0) {
                return EIO;
            }
            if (this->blockDev->read(currentBlock, dmapBlock) != 0) {
                return EIO;
            }
            lastBlock = currentBlock;
        }
        dmapBlock[(arr[i] - FILES_INDEX) % BD_BLOCK_SIZE] = 'A';
    }
    if (this->blockDev->write(lastBlock, dmapBlock) != 0) {
        return EIO;
    }
    return 0;
}

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
 * Object representing the FAT sector of the container.
 */
FAT::FAT(BlockDevice *blockDev) { this->blockDev = blockDev; }

FAT::~FAT() {}

/**
 * Reads next address from the given address.
 *
 * @return     the next address.
 */
uint16_t FAT::read(uint16_t curAddress) {
#ifdef DEBUG
    fprintf(stderr, "Read address %d from Fat\n", curAddress);
#endif
    uint16_t blockAddr = (curAddress - FILES_INDEX) / 256 + FAT_INDEX;
    uint16_t index = (curAddress - FILES_INDEX) % 256;
    uint16_t *fatBlock = (uint16_t *)malloc(BD_BLOCK_SIZE);

    blockDev->read(blockAddr, (char *)fatBlock);
    uint16_t res = fatBlock[index];
    free(fatBlock);
    return res;
}

/**
 * Writes next address to the given address.
 * 
 * @return  0 when successful.
 *          EIO when write failed.
 */
int FAT::write(uint16_t curAddress, uint16_t nextAddress) {
#ifdef DEBUG
    fprintf(stderr, "Write address %d -> %d to Fat\n", curAddress, nextAddress);
#endif
    uint16_t blockAddr = (curAddress - FILES_INDEX) / 256 + FAT_INDEX;
    uint16_t index = (curAddress - FILES_INDEX) % 256;
    uint16_t *fatBlock = (uint16_t *)malloc(BD_BLOCK_SIZE);

    this->blockDev->read(blockAddr, (char *)fatBlock);
    fatBlock[index] = nextAddress;
    if (this->blockDev->write(blockAddr, (char *)fatBlock) != 0) {
        free(fatBlock);
        return EIO;
    }
    free(fatBlock);
    return 0;
}

/**
 * Object representing the RootDir sector of the container.
 */
RootDir::RootDir(BlockDevice *blockDev, int fileCount) {
    this->blockDev = blockDev;
    this->fileCount = fileCount;
}

RootDir::~RootDir() {}

/**
 * Returns the amount of files currently in the filesystem.
 */
int RootDir::len() { return this->fileCount; }

/**
 * Gets the fileinformation by filename.
 *
 * @param *name     The name of the file.
 * @param *fileData The file information will be stored in here.
 *
 * @return          0 when the file was found.
 *                  ENOENT when the file wasn't found.
 *                  EIO when read failed.
 */
int RootDir::get(const char *name, dpsFile *fileData) {
#ifdef DEBUG
    fprintf(stderr, "get fileInfo by name for %s from RootDir\n", name);
#endif
    for (int i = 0; i < this->len(); i++) {
        if (this->read(i, fileData) != 0) {
            return EIO;
        }
        if (strcmp(fileData->name, name) == 0) {
            return 0;
        }
    }
    return ENOENT;
}

/**
 * Checks if a file exists in the filesystem.
 *
 * @param *name     The name of the file.
 *
 * @return          0 when the file exists.
 *                  ENOENT when the file doesn't exist.
 *                  EIO when read failed.
 */
int RootDir::exists(const char *name) {
#ifdef DEBUG
    fprintf(stderr, "check if file exists: %s\n", name);
#endif
    int res;
    dpsFile *tmp = (dpsFile *)malloc(BD_BLOCK_SIZE);
    res = this->get(name, tmp);
    free(tmp);
    return res;
}

/**
 * Reads file information from the RootDir.
 *
 * @param num       The number under which the file was stored (0..64).
 * @param *fileData The file information will be stored in here.
 *
 * @return          EFAULT when num was bigger than NUM_DIR_ENTRIES or negativ.
 *                  EIO when read failed.
 *                  0 otherwise.
 */
int RootDir::read(uint16_t num, dpsFile *fileData) {
#ifdef DEBUG
    fprintf(stderr, "read block %d from RoodDir\n", num);
#endif
    if (num > NUM_DIR_ENTRIES || num < 0) {
        return EFAULT;
    }
    if (this->blockDev->read(ROOTDIR_INDEX + num, (char *)fileData) != 0) {
        return EIO;
    }
    return 0;
}

/**
 * Writes file information to the RootDir.
 *
 * @param num       The number under which the file is stored (0..64).
 * @param *fileData The file information which must be stored.
 *
 * @return          EFAULT when num was bigger than NUM_DIR_ENTRIES or negativ.
 *                  EIO when write failed.
 *                  0 otherwise.
 */
int RootDir::write(uint16_t num, dpsFile *fileData) {
#ifdef DEBUG
    fprintf(stderr, "write block %d to RoodDir\n", num);
#endif
    if (num > NUM_DIR_ENTRIES || num < 0) {
        return EFAULT;
    }
    fileData->stat.st_atime = time(NULL);
    fileData->stat.st_ctime = time(NULL);
    fileData->stat.st_uid = getgid();
    fileData->stat.st_gid = getuid();
    fileData->stat.st_mode = S_IFREG | 0444;

    if (this->blockDev->write(ROOTDIR_INDEX + num, (char *)fileData) != 0) {
        return EIO;
    }
    this->fileCount++;
    return 0;
}
