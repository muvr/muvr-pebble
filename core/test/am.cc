#include <gtest/gtest.h>
#include "am.h"
#include "mock.h"

using namespace pebble::mock;

class am_test : public testing::Test {

};

TEST_F(am_test, empty) {
    auto callback = am_start(123, 100, 5);
    callback(nullptr, 0);
}