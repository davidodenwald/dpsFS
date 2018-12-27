#include "catch.hpp"
#include "container.h"

#define CONTAINER "/tmp/bd.bin"

TEST_CASE("create/getFree/allocate/toFile", "[DMAP]") {
    fclose(fopen(CONTAINER, "w"));
    BlockDevice blockDev = BlockDevice();
    blockDev.open(CONTAINER);
    DMAP dmap = DMAP(&blockDev);

    SECTION("dmap single block") {
        dmap.create();
    
        uint16_t pos = 0;
        int err = dmap.getFree(&pos);
        REQUIRE(err == 0);
        REQUIRE(pos == FILES_INDEX);

        err = dmap.allocate(pos);
        REQUIRE(err == 0);

        err = dmap.getFree(&pos);
        REQUIRE(err == 0);
        REQUIRE(pos == FILES_INDEX + 1);
    }

    SECTION("dmap multiple blocks") {
        dmap.create();
        
        // test with the biggest possible filesize
        uint16_t arr[FILES_SIZE];
        int err = dmap.getFree(FILES_SIZE, arr);
        REQUIRE(err == 0);
        REQUIRE(arr[0] == FILES_INDEX);
        REQUIRE(arr[FILES_SIZE / 2] == FILES_INDEX + FILES_SIZE / 2);
        REQUIRE(arr[FILES_SIZE - 1] == FILES_INDEX + FILES_SIZE - 1);

        err = dmap.allocate(FILES_SIZE, arr);
        REQUIRE(err == 0);

        // after allocating the biggest possible file, there is no space left
        uint16_t arr2[5];
        err = dmap.getFree(5, arr2);
        REQUIRE(err != 0);
    }

    SECTION("toFile") {
        dmap.create();

        uint16_t pos = 0;
        dmap.getFree(&pos);
        REQUIRE(pos == FILES_INDEX);
        dmap.allocate(pos);
        dmap.toFile();
        
        DMAP dmap2 = DMAP(&blockDev);
        dmap2.getFree(&pos);
        REQUIRE(pos == FILES_INDEX + 1);
    }
}