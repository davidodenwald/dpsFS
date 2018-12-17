#include "catch.hpp"
#include "container.h"

#define CONTAINER "/tmp/bd.bin"

TEST_CASE("create/getFree/allocate", "[DMAP]") {
    fclose(fopen(CONTAINER, "w"));
    BlockDevice blockDev = BlockDevice();
    blockDev.open(CONTAINER);
    DMAP dmap = DMAP(&blockDev);

    int err = dmap.create();
    REQUIRE(err == 0);

    // test with the biggest possible filesize
    uint16_t arr[FILES_SIZE];
    int num = dmap.getFree(FILES_SIZE, arr);
    REQUIRE(num == FILES_SIZE);
    REQUIRE(arr[0] == FILES_INDEX);
    REQUIRE(arr[FILES_SIZE/2] == FILES_INDEX + FILES_SIZE/2);
    REQUIRE(arr[FILES_SIZE - 1] == FILES_INDEX + FILES_SIZE - 1);

    err = dmap.allocate(FILES_SIZE, arr);
    REQUIRE(err == 0);
    
    // after allocating the biggest possible file, there is no space left
    uint16_t arr2[5];
    num = dmap.getFree(5, arr2);
    REQUIRE(num == 0);
}
