#ifndef container_h
#define container_h

#include <sys/stat.h>

#include "blockdevice.h"
#include "myfs-structs.h"

#define SUPERBLOCK_SIZE 1
#define SUPERBLOCK_INDEX 0
#define DMAP_SIZE 128
#define DMAP_INDEX 1
#define FAT_SIZE 256
#define FAT_INDEX 129
#define ROOTDIR_SIZE 64
#define ROOTDIR_INDEX 385
#define FILES_SIZE 65087
#define FILES_INDEX 449

struct sbStats {
    uint16_t fileCount;
};

class Superblock {
   private:
    BlockDevice *blockDev;

   public:
    Superblock(BlockDevice *blockDev);
    ~Superblock();
    int read(sbStats *content);
    int write(sbStats *content);
};

class DMAP {
   private:
    BlockDevice *blockDev;
    char *dmap;

   public:
    DMAP(BlockDevice *blockDev);
    ~DMAP();
    int getFree(uint16_t *pos);
    int getFree(uint16_t num, uint16_t *arr);
    int allocate(uint16_t pos);
    int allocate(uint16_t num, uint16_t *arr);
};

class FAT {
   private:
    BlockDevice *blockDev;
    uint16_t *fat;

   public:
    FAT(BlockDevice *blockDev);
    ~FAT();
    uint16_t read(uint16_t blockPos);
    int write(uint16_t curAddress, uint16_t nextAddress);
};

struct dpsFile {
    char name[NAME_LENGTH];
    struct stat stat;
    uint16_t firstBlock;
};

class RootDir {
   private:
    BlockDevice *blockDev;
    int fileCount;

   public:
    RootDir(BlockDevice *blockDev, int fileCount);
    ~RootDir();
    int len();
    int get(const char *name, dpsFile *fileData);
    int get(const char *name, dpsFile *fileData, uint16_t *num);
    int exists(const char *name);
    int read(uint16_t num, dpsFile *fileData);
    int write(uint16_t num, dpsFile *fileData);
};

class Files {
   private:
    BlockDevice *blockDev;

   public:
    Files(BlockDevice *blockDev);
    ~Files();
    int read(uint16_t *blocks, uint16_t num, uint16_t offset, char *buf);
    int write(uint16_t *blocks, uint16_t num, uint16_t offset, size_t size,
              const char *buf);
};

#endif /* container_h */