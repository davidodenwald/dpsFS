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
    void allocate(unsigned short num, unsigned short *arr);
    void getFree(unsigned short num, unsigned short *arr);
};

class FAT {
   private:
    BlockDevice *blockDev;

   public:
    FAT(BlockDevice *blockDev);
    ~FAT();
    void write(unsigned short curAddress, unsigned short nextAddress);
    unsigned short read(unsigned short blockPos);
};

struct dpsFile {
    char name[256];
    struct stat stat;
    unsigned short firstBlock;
};

class RootDir {
   private:
    BlockDevice *blockDev;

   public:
    RootDir(BlockDevice *blockDev);
    ~RootDir();
    void write(unsigned short num, dpsFile *fileData);
    void read(unsigned short num, dpsFile *fileData);
};

#endif