#include "catch.hpp"
#include "container.h"

#define CONTAINER "/tmp/bd.bin"

TEST_CASE("Write/Read block", "[SuperBlock]") {
    fclose(fopen(CONTAINER, "w"));
    BlockDevice blockDev = BlockDevice();
    blockDev.open(CONTAINER);
    Superblock superBlock = Superblock(&blockDev);

    sbStats *s1 = (sbStats *)malloc(BD_BLOCK_SIZE);
    s1->fileCount = 500;
    int err = superBlock.write(s1);
    REQUIRE(err == 0);

    sbStats *s2 = (sbStats *)malloc(BD_BLOCK_SIZE);
    err = superBlock.read(s2);
    REQUIRE(err == 0);
    REQUIRE(s2->fileCount == s1->fileCount);

    free(s1);
    free(s2);
}
