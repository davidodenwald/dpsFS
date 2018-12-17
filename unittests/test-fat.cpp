#include "catch.hpp"
#include "container.h"

#define CONTAINER "/tmp/bd.bin"

TEST_CASE("read/write", "[fat]") {
    fclose(fopen(CONTAINER, "w"));
    BlockDevice blockDev = BlockDevice();
    blockDev.open(CONTAINER);
    FAT fat = FAT(&blockDev);
    
    int err = fat.write(449, 50000);
    REQUIRE(err == 0);
    REQUIRE(fat.read(449) == 50000);

    err = fat.write(50000, 450);
    REQUIRE(err == 0);
    REQUIRE(fat.read(50000) == 450);

    err = fat.write(450, 0);
    REQUIRE(err == 0);
    REQUIRE(fat.read(450) == 0);
}
