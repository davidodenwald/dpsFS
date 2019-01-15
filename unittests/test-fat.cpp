#include "catch.hpp"
#include "container.h"

#define CONTAINER "/tmp/bd.bin"

TEST_CASE("read/write/toFile", "[fat]") {
    fclose(fopen(CONTAINER, "w"));
    BlockDevice blockDev = BlockDevice();
    blockDev.open(CONTAINER);
    FAT fat = FAT(&blockDev);

    SECTION("read/write") {
        fat.write(449, 50000);
        REQUIRE(fat.read(449) == 50000);

        fat.write(50000, 450);
        REQUIRE(fat.read(50000) == 450);

        fat.write(450, 0);
        REQUIRE(fat.read(450) == 0);
    }

    SECTION("toFile") {
        int err = fat.toFile();
        REQUIRE(err == 0);

        FAT fat2 = FAT(&blockDev);
        REQUIRE(fat2.read(449) == 50000);
        REQUIRE(fat2.read(50000) == 450);
        REQUIRE(fat2.read(450) == 0);
    }
}
