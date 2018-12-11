#ifndef container_h
#define container_h

#include <sys/stat.h>
#include "blockdevice.h"

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

bool checkBoundary(int address);

class Superblock {
   private:
    BlockDevice *blockDev;

   public:
    Superblock(BlockDevice *blockDev);
    ~Superblock();
    void write(char *content);
    void read(char *content);
};

class DMAP {
   private:
    BlockDevice *blockDev;

   public:
    DMAP(BlockDevice *blockDev);
    ~DMAP();
    void create();
    void allocate(uint16_t num, uint16_t *arr);
    void getFree(uint16_t num, uint16_t *arr);
};

class FAT {
   private:
    BlockDevice *blockDev;

   public:
    FAT(BlockDevice *blockDev);
    ~FAT();
    void write(uint16_t curAddress, uint16_t nextAddress);
    uint16_t read(uint16_t blockPos);
};

struct dpsFile {
    char name[256];
    struct stat stat;
    uint16_t firstBlock;
};

class RootDir {
   private:
    BlockDevice *blockDev;

   public:
    RootDir(BlockDevice *blockDev);
    ~RootDir();
    int write(uint16_t num, dpsFile *fileData);
    int read(uint16_t num, dpsFile *fileData);
    int get(const char *name, dpsFile *fileData);
};

#endif