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
DMAP::DMAP(BlockDevice *blockDev) {
    this->blockDev = blockDev;
    this->dmap = (char *)malloc(DMAP_SIZE * BD_BLOCK_SIZE);

    for (int i = 0; i < DMAP_SIZE; i++) {
        this->blockDev->read(i + DMAP_INDEX, this->dmap);
        this->dmap += BD_BLOCK_SIZE;
    }
    this->dmap -= DMAP_SIZE * BD_BLOCK_SIZE;
}

DMAP::~DMAP() { free(this->dmap); }

/**
 * Initializes the DMAP sector.
 */
void DMAP::create() {
    memset(this->dmap, 0, DMAP_SIZE * BD_BLOCK_SIZE);
    memset(this->dmap, 'F', FILES_SIZE);
}

/**
 * Gets a free block.
 *
 * @param pos   The variable in which the block is stored.
 *
 * @return      0 if successful.
 *              ENOSPC when no free block is available.
 */
int DMAP::getFree(uint16_t *pos) {
#ifdef DEBUG
    fprintf(stderr, "Get Free block from DMAP\n");
#endif
    for (int i = 0; i < FILES_SIZE; i++) {
        if (this->dmap[i] == 'F') {
            *pos = i + FILES_INDEX;
            return 0;
        }
    }
    return ENOSPC;
}

/**
 * Gets free blocks.
 *
 * @param num   The number of blocks requested.
 * @param *arr  The array in which the blocks are stored.
 *
 * @return      0 if successful.
 *              ENOSPC when not enough free blocks are available.
 */
int DMAP::getFree(uint16_t num, uint16_t *arr) {
#ifdef DEBUG
    fprintf(stderr, "Get Free %d blocks from DMAP\n", num);
#endif
    uint16_t found = 0;
    for (int i = 0; i < FILES_SIZE; i++) {
        if (found == num) {
            return 0;
        }
        if (this->dmap[i] == 'F') {
            arr[found] = i + FILES_INDEX;
            found++;
        }
    }
    if (found == num) {
        return 0;
    }
    return ENOSPC;
}

/**
 * Marks the given block as allocated.
 *
 * @param pos  The block which must be written.
 *
 *  @return     0 when successful.
 */
int DMAP::allocate(uint16_t pos) {
#ifdef DEBUG
    fprintf(stderr, "Allocate block %d in DMAP\n", pos);
#endif
    this->dmap[pos - FILES_INDEX] = 'A';
    return 0;
}

/**
 * Marks the given blocks as allocated.
 *
 * @param num   The number of blocks that must be written.
 * @param *arr  The array of block which must be written.
 *
 *  @return     0 when successful.
 */
int DMAP::allocate(uint16_t num, uint16_t *arr) {
#ifdef DEBUG
    fprintf(stderr, "Allocate %d blocks in DMAP\n", num);
#endif
    for (uint16_t i = 0; i < num; i++) {
        this->dmap[arr[i] - FILES_INDEX] = 'A';
    }
    return 0;
}

int DMAP::toFile() {
    for (int i = 0; i < DMAP_SIZE; i++) {
        if (this->blockDev->write(i + DMAP_INDEX, this->dmap) != 0) {
            return EIO;
        }
        this->dmap += BD_BLOCK_SIZE;
    }
    this->dmap -= DMAP_SIZE * BD_BLOCK_SIZE;
    return 0;
}

/**
 * Object representing the FAT sector of the container.
 */
FAT::FAT(BlockDevice *blockDev) {
    this->blockDev = blockDev;
    this->fat = (uint16_t *)malloc(FAT_SIZE * BD_BLOCK_SIZE);

    for (int i = 0; i < FAT_SIZE; i++) {
        this->blockDev->read(i + FAT_INDEX, (char *)this->fat);
        this->fat += BD_BLOCK_SIZE / 2;
    }
    this->fat -= FAT_SIZE * BD_BLOCK_SIZE / 2;
}

FAT::~FAT() { free(this->fat); }

/**
 * Reads next address from the given address.
 *
 * @return     the next address.
 */
uint16_t FAT::read(uint16_t curAddress) {
#ifdef DEBUG
    fprintf(stderr, "Read address %d from Fat\n", curAddress);
#endif
    return this->fat[curAddress - FILES_INDEX];
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
    this->fat[curAddress - FILES_INDEX] = nextAddress;
    return 0;
}

int FAT::toFile() {
    for (int i = 0; i < FAT_SIZE; i++) {
        if (this->blockDev->write(i + FAT_INDEX, (char *)this->fat) != 0) {
            return EIO;
        }
        this->fat += BD_BLOCK_SIZE / 2;
    }
    this->fat -= FAT_SIZE * BD_BLOCK_SIZE / 2;
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
 * Gets the fileinformation and number of entry by filename.
 * The number can be used for overwriting this entry.
 *
 * @param *name     The name of the file.
 * @param *num      The file number will be stored in here.
 *
 * @return          0 when the file was found.
 *                  ENOENT when the file wasn't found.
 *                  EIO when read failed.
 */
int RootDir::get(const char *name, dpsFile *fileData, uint16_t *num) {
#ifdef DEBUG
    fprintf(stderr, "get fileInfo by name for %s from RootDir\n", name);
#endif
    for (int i = 0; i < this->len(); i++) {
        if (this->read(i, fileData) != 0) {
            return EIO;
        }
        if (strcmp(fileData->name, name) == 0) {
            *num = i;
            return 0;
        }
    }
    return ENOENT;
}

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

    if (this->exists(fileData->name) != 0) {
        this->fileCount++;
    }

    if (this->blockDev->write(ROOTDIR_INDEX + num, (char *)fileData) != 0) {
        return EIO;
    }
    return 0;
}

Files::Files(BlockDevice *blockDev) {
    this->blockDev = blockDev;
    this->buffer = (char *)malloc(BD_BLOCK_SIZE);
    this->bufIndex = 0;
}

Files::~Files() { free(this->buffer); }

int Files::read(uint16_t *blocks, uint16_t num, uint16_t offset, char *buf) {
    if (num == 1 && blocks[0] == this->bufIndex) {
        memcpy(buf, this->buffer, BD_BLOCK_SIZE);
    }
    for (int i = 0; i < num; i++) {
        if (this->blockDev->read(blocks[i], buf) != 0) {
            memcpy(this->buffer, buf, BD_BLOCK_SIZE);
            this->bufIndex = blocks[i];
            return EIO;
        }
        buf += BD_BLOCK_SIZE;
    }
    buf -= BD_BLOCK_SIZE * num - offset;
    return 0;
}

int Files::write(uint16_t *blocks, uint16_t num, uint16_t offset, size_t size,
                 const char *buf) {
    for (int i = 0; i < num; i++) {
        memset(this->buffer, 0, BD_BLOCK_SIZE);
        this->blockDev->read(blocks[i], this->buffer);
        if (i == 0) {
            this->buffer += offset;
        }

        int written = 0;
        while ((size_t)written < size && written < BD_BLOCK_SIZE) {
            *this->buffer = *buf;
            buf++;
            this->buffer++;
            written++;
        }
        if (i == 0) {
            this->buffer -= written + offset;
        } else {
            this->buffer -= written;
        }
        this->blockDev->write(blocks[i], this->buffer);
        size -= BD_BLOCK_SIZE;
    }
    return 0;
}