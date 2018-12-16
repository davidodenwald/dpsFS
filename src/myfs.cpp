//
//  myfs.cpp
//  myfs
//
//  Created by Oliver Waldhorst on 02.08.17.
//  Copyright © 2017 Oliver Waldhorst. All rights reserved.
//

// For documentation of FUSE methods see
// https://libfuse.github.io/doxygen/structfuse__operations.html

#undef DEBUG

// TODO: Comment this to reduce debug messages
#define DEBUG
#define DEBUG_METHODS
#define DEBUG_RETURN_VALUES

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "blockdevice.h"
#include "container.h"
#include "macros.h"
#include "myfs-info.h"
#include "myfs.h"

MyFS *MyFS::_instance = NULL;

MyFS *MyFS::Instance() {
    if (_instance == NULL) {
        _instance = new MyFS();
    }
    return _instance;
}

MyFS::MyFS() {
    this->logFile = stderr;
    this->openFiles = 0;
    this->blockDev = new BlockDevice();
    this->superBlock = new Superblock(this->blockDev);
    this->dmap = new DMAP(this->blockDev);
    this->fat = new FAT(this->blockDev);
    // this->rootDir is initialized in MyFS::fuseInit.
}

MyFS::~MyFS() {
    this->blockDev->close();
    delete this->blockDev;
    delete this->superBlock;
    delete this->dmap;
    delete this->fat;
    delete this->rootDir;
}

int MyFS::fuseGetattr(const char *path, struct stat *statbuf) {
    LOGM();
    LOGF("path: %s", path);

    if (strcmp(path, "/") == 0) {
        statbuf->st_uid = getuid();
        statbuf->st_gid = getgid();
        statbuf->st_atime = time(NULL);
        statbuf->st_mtime = time(NULL);
        statbuf->st_ctime = time(NULL);
        statbuf->st_mode = S_IFDIR | 0555;
        statbuf->st_nlink = 2;
        statbuf->st_size = 4096;
        statbuf->st_blocks = 8;
        RETURN(0);
    }

    const char *name = basename(path);
    if (rootDir->exists(name) != 0) {
        RETURN(-ENOENT);
    }

    dpsFile *tmpFile = (dpsFile *)malloc(BD_BLOCK_SIZE);
    int err = rootDir->get(name, tmpFile);
    memcpy(statbuf, &tmpFile->stat, sizeof(*statbuf));
    free(tmpFile);

    RETURN(-err);
}

int MyFS::fuseReadlink(const char *path, char *link, size_t size) {
    LOGM();
    return 0;
}

int MyFS::fuseMknod(const char *path, mode_t mode, dev_t dev) {
    LOGM();

    // TODO: Implement this!

    RETURN(0);
}

int MyFS::fuseMkdir(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseUnlink(const char *path) {
    LOGM();

    // TODO: Implement this!

    RETURN(0);
}

int MyFS::fuseRmdir(const char *path) {
    LOGM();
    return 0;
}

int MyFS::fuseSymlink(const char *path, const char *link) {
    LOGM();
    return 0;
}

int MyFS::fuseRename(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseLink(const char *path, const char *newpath) {
    LOGM();
    return 0;
}

int MyFS::fuseChmod(const char *path, mode_t mode) {
    LOGM();
    return 0;
}

int MyFS::fuseChown(const char *path, uid_t uid, gid_t gid) {
    LOGM();
    return 0;
}

int MyFS::fuseTruncate(const char *path, off_t newSize) {
    LOGM();
    return 0;
}

int MyFS::fuseUtime(const char *path, struct utimbuf *ubuf) {
    LOGM();
    return 0;
}

int MyFS::fuseOpen(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();

    this->openFiles++;
    if (this->openFiles > NUM_OPEN_FILES) {
        this->openFiles--;
        LOGF("error: cannot open file %s. Limit exceeded.\n", path);
        RETURN(-EMFILE);
    }
    dpsFile *tmpFile = (dpsFile *)malloc(BD_BLOCK_SIZE);
    int err = this->rootDir->get(basename(path), tmpFile);
    if (err == 0) {
        fileInfo->fh = tmpFile->firstBlock;
    }
    free(tmpFile);
    RETURN(-err);
}

int MyFS::fuseRead(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fileInfo) {
    LOGM();
    LOGF("Path: %s | Size: %ld | Offset: %ld\n", path, size, offset);
    
    uint16_t block;
    int read = 0;
    block = fileInfo->fh;
    char *tmpBuf = (char *)malloc(BD_BLOCK_SIZE);
    
    while((size_t) read < (size + offset)) {
        memset(tmpBuf, 0, BD_BLOCK_SIZE);
        read += BD_BLOCK_SIZE;
        if (read < offset) {
            block = this->fat->read(block);
            continue;
        }
        if (block == 0) {
            break;
        }
        this->blockDev->read(block, tmpBuf);
        block = this->fat->read(block);
        memcpy(buf, tmpBuf, BD_BLOCK_SIZE);
        buf += BD_BLOCK_SIZE;
    }
    free(tmpBuf);
    
    // rewind buf
    buf -= read - (int) offset;
    LOGF("read bytes: %d\n",read - (int) offset);
    RETURN(read - (int) offset);
}

int MyFS::fuseWrite(const char *path, const char *buf, size_t size,
                    off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: Implement this!

    RETURN(0);
}

int MyFS::fuseStatfs(const char *path, struct statvfs *statInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseFlush(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    return 0;
}

int MyFS::fuseRelease(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    this->openFiles--;
    RETURN(0);
}

int MyFS::fuseFsync(const char *path, int datasync, struct fuse_file_info *fi) {
    LOGM();
    return 0;
}

int MyFS::fuseListxattr(const char *path, char *list, size_t size) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseRemovexattr(const char *path, const char *name) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseOpendir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseReaddir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fileInfo) {
    LOGM();
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    if (strcmp(path, "/") == 0) {
        dpsFile *tmpFile = (dpsFile *)malloc(BD_BLOCK_SIZE);
        for (int i = 0; i < rootDir->len(); i++) {
            rootDir->read(i, tmpFile);
            filler(buf, tmpFile->name, &tmpFile->stat, 0);
        }
        free(tmpFile);
    }
    RETURN(0);
}

int MyFS::fuseReleasedir(const char *path, struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseFsyncdir(const char *path, int datasync,
                       struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseTruncate(const char *path, off_t offset,
                       struct fuse_file_info *fileInfo) {
    LOGM();
    RETURN(0);
}

int MyFS::fuseCreate(const char *path, mode_t mode,
                     struct fuse_file_info *fileInfo) {
    LOGM();

    // TODO: Implement this!

    RETURN(0);
}

void MyFS::fuseDestroy() { LOGM(); }

void *MyFS::fuseInit(struct fuse_conn_info *conn) {
    // Open logfile
    this->logFile =
        fopen(((MyFsInfo *)fuse_get_context()->private_data)->logFile, "w+");
    if (this->logFile == NULL) {
        fprintf(stderr, "ERROR: Cannot open logfile %s\n",
                ((MyFsInfo *)fuse_get_context()->private_data)->logFile);
    } else {
        // turn of logfile buffering
        setvbuf(this->logFile, NULL, _IOLBF, 0);

        LOG("Starting logging...\n");
        LOGM();

        LOGF("Container file name: %s",
             ((MyFsInfo *)fuse_get_context()->private_data)->contFile);

        blockDev->open(
            ((MyFsInfo *)fuse_get_context()->private_data)->contFile);

        sbStats *s = (sbStats *)malloc(BD_BLOCK_SIZE);
        if (this->superBlock->read(s) != 0) {
            LOG("Could not read Superblock");
        }
        this->rootDir = new RootDir(this->blockDev, s->fileCount);
        free(s);
    }
    RETURN(0);
}

#ifdef __APPLE__
int MyFS::fuseSetxattr(const char *path, const char *name, const char *value,
                       size_t size, int flags, uint32_t x) {
#else
int MyFS::fuseSetxattr(const char *path, const char *name, const char *value,
                       size_t size, int flags) {
#endif
    LOGM();
    RETURN(0);
}

#ifdef __APPLE__
int MyFS::fuseGetxattr(const char *path, const char *name, char *value,
                       size_t size, uint x) {
#else
int MyFS::fuseGetxattr(const char *path, const char *name, char *value,
                       size_t size) {
#endif
    LOGM();
    RETURN(0);
}

// TODO: Add your own additional methods here!
