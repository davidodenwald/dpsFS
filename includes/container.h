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

class Superblock {
   private:
    BlockDevice *blockDev;

   public:
    Superblock(BlockDevice *blockDev);
    ~Superblock();
};

class DMAP {
   private:
    BlockDevice *blockDev;
    char *dmap;

   public:
    DMAP(BlockDevice *blockDev);
    ~DMAP();
    void create();
    int getFree(uint16_t *pos);
    int getFree(uint16_t num, uint16_t *arr);
    void setFree(uint16_t pos);
    void allocate(uint16_t pos);
    void allocate(uint16_t num, uint16_t *arr);
    int toFile();
};

class FAT {
   private:
    BlockDevice *blockDev;
    uint16_t *fat;

   public:
    FAT(BlockDevice *blockDev);
    ~FAT();
    uint16_t read(uint16_t blockPos);
    void write(uint16_t curAddress, uint16_t nextAddress);
    int toFile();
};

struct dpsFile {
    char name[NAME_LENGTH];
    struct stat stat;
    uint16_t firstBlock;
};

class RootDir {
   private:
    BlockDevice *blockDev;
    dpsFile *files;

   public:
    RootDir(BlockDevice *blockDev);
    ~RootDir();
    int get(const char *name, dpsFile *fileData);
    int exists(const char *name);
    int read(uint16_t num, dpsFile *fileData);
    int write(dpsFile *fileData);
    int del(const char *name);
    int toFile();
};

class Files {
   private:
    BlockDevice *blockDev;
    char *buffer;
    int bufIndex;

   public:
    Files(BlockDevice *blockDev);
    ~Files();
    int read(uint16_t *blocks, uint16_t num, uint16_t offset, char *buf);
    int write(uint16_t *blocks, uint16_t num, uint16_t offset, size_t size,
              const char *buf);
};

#endif /* container_h */