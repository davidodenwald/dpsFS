#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "container.h"
#include "myfs-structs.h"

/**
 * Object representing the Superblock sector of the container.
 */
Superblock::Superblock(BlockDevice *blockDev) { this->blockDev = blockDev; }

Superblock::~Superblock() {}

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
    uint16_t found = 0;
    for (int i = 0; i < FILES_SIZE; i++) {
        if (this->dmap[i] == 'F') {
            arr[found] = i + FILES_INDEX;
            found++;
        }
        if (found == num) {
            return 0;
        }
    }
    return ENOSPC;
}

/**
 * Sets a given Block to "free".
 *
 *  @param pos   the block that is to be set to free.
 */
void DMAP::setFree(uint16_t pos) {
    this->dmap[pos - FILES_INDEX] = 'F';
}

/**
 * Marks the given block as allocated.
 *
 * @param pos   The block which must be written.
 */
void DMAP::allocate(uint16_t pos) {
    this->dmap[pos - FILES_INDEX] = 'A';
}

/**
 * Marks the given blocks as allocated.
 *
 * @param num   The number of blocks that must be written.
 * @param *arr  The array of block which must be written.
 */
void DMAP::allocate(uint16_t num, uint16_t *arr) {
    for (uint16_t i = 0; i < num; i++) {
        this->dmap[arr[i] - FILES_INDEX] = 'A';
    }
}

/**
 * Writes the DMAP back to the container file.
 *
 * @return  0 when successful
 *          EIO when write failed
 */
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
    return this->fat[curAddress - FILES_INDEX];
}

/**
 * Writes next address to the given address.
 */
void FAT::write(uint16_t curAddress, uint16_t nextAddress) {
    this->fat[curAddress - FILES_INDEX] = nextAddress;
}

/**
 * Writes the FAT back to the container file.
 *
 * @return  0 when successful
 *          EIO when write failed
 */
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
RootDir::RootDir(BlockDevice *blockDev) {
    this->blockDev = blockDev;
    this->files = (dpsFile *)malloc(ROOTDIR_SIZE * sizeof(dpsFile));
    memset(this->files, 0, ROOTDIR_SIZE * sizeof(dpsFile));
    char *tmpBuf = (char *)malloc(BD_BLOCK_SIZE);

    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        this->blockDev->read(i + ROOTDIR_INDEX, tmpBuf);
        memcpy(&this->files[i], tmpBuf, sizeof(dpsFile));
    }
    free(tmpBuf);
}

RootDir::~RootDir() { free(files); }

/**
 * Gets the fileinformation by filename.
 *
 * @param *name     The name of the file.
 * @param *fileData The file information will be stored in here.
 *
 * @return          0 when the file was found.
 *                  ENOENT when the file wasn't found.
 */
int RootDir::get(const char *name, dpsFile *fileData) {
    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        if (strcmp(this->files[i].name, name) == 0) {
            memcpy(fileData, &this->files[i], sizeof(dpsFile));
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
 */
int RootDir::exists(const char *name) {
    dpsFile *tmp = (dpsFile *)malloc(BD_BLOCK_SIZE);
    int err = this->get(name, tmp);
    free(tmp);
    return err;
}

/**
 * Reads file information from the RootDir.
 *
 * @param num       The number under which the file was stored (0..64).
 * @param *fileData The file information will be stored in here.
 *
 * @return          EFAULT when num was bigger than NUM_DIR_ENTRIES or negativ.
 *                  0 otherwise.
 */
int RootDir::read(uint16_t num, dpsFile *fileData) {
    if (num > NUM_DIR_ENTRIES || num < 0) {
        return EFAULT;
    }
    memcpy(fileData, &files[num], sizeof(dpsFile));
    return 0;
}

/**
 * Writes file information to the RootDir.
 *
 * @param *fileData The file information which must be stored.
 *
 * @return          EFAULT when files in the Filesystem exceed NUM_DIR_ENTRIES.
 *                  0 otherwise.
 */
int RootDir::write(dpsFile *fileData) {
    // overwrite file if it exists
    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        if (strcmp(this->files[i].name, fileData->name) == 0) {
            memcpy(&this->files[i], fileData, sizeof(dpsFile));
            return 0;
        }
    }

    // write to empty slot
    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        if (this->files[i].name[0] == 0) {
            memcpy(&this->files[i], fileData, sizeof(dpsFile));
            return 0;
        }
    }
    return EFAULT;
}
/**
 * Deletes a file.
 *
 * @param *name The filename.
 *
 * @return      ENOENT when the file doesn't exist.
 *              0 when successful.
 */
int RootDir::del(const char *name) {
    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        if (strcmp(this->files[i].name, name) == 0) {
            memset(&this->files[i], 0, sizeof(dpsFile));
            return 0;
        }
    }
    return ENOENT;
}

/**
 * Writes the RootDir back to the container file.
 *
 * @return  0 when successful
 *          EIO when write failed
 */
int RootDir::toFile() {
    char *tmpBuf = (char *)malloc(BD_BLOCK_SIZE);
    for (int i = 0; i < ROOTDIR_SIZE; i++) {
        memcpy(tmpBuf, &this->files[i], sizeof(dpsFile));
        if (this->blockDev->write(i + ROOTDIR_INDEX, tmpBuf) != 0) {
            return EIO;
        }
    }
    return 0;
}

/**
 * Object for reading and writing to the files sector of the container file.
 */
Files::Files(BlockDevice *blockDev) {
    this->blockDev = blockDev;
    this->buffer = (char *)malloc(BD_BLOCK_SIZE);
    this->bufIndex = 0;
}

Files::~Files() { free(this->buffer); }

/**
 * Reads a number of blocks from the container file.
 *
 * @param *blocks   the blocknumbers to be read
 * @param num       the number of blocks to be read
 * @param offset    the offset at which to read
 * @param *buf      the buffer into which the bytes are written
 *
 * @return          0 when successful
 *                  EIO when read failed
 */
int Files::read(uint16_t *blocks, uint16_t num, uint16_t offset, char *buf) {
    if (num == 1 && blocks[0] == this->bufIndex) {
        memcpy(buf, this->buffer, BD_BLOCK_SIZE);
    }
    for (int i = 0; i < num; i++) {
        if (this->blockDev->read(blocks[i], buf) != 0) {
            return EIO;
        }
        memcpy(this->buffer, buf, BD_BLOCK_SIZE);
        this->bufIndex = blocks[i];
        buf += BD_BLOCK_SIZE;
    }
    buf -= BD_BLOCK_SIZE * num - offset;
    return 0;
}

/**
 * Writes a number of blocks from the container file.
 *
 * @param *blocks   the block numbers to be written
 * @param num       the number of blocks to be written
 * @param offset    the offset at which to write
 * @param size      the number of bytes to be written
 * @param *buf      the buffer from which the bytes are written
 *
 * @return          0 when successful
 *                  EIO when read or write failed
 */
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