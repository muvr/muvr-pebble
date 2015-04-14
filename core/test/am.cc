#include <gtest/gtest.h>
#include "am.h"
#include "instances.h"

class am_test : public testing::Test {

};

TEST_F(am_test, empty) {
    auto callback = am_start(123, 100, 5);
    uint8_t buf[] = {1, 2, 3};
    callback(buf, 3);
    pebble::instances::app_messages.last_dict();
}