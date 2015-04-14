#include <gtest/gtest.h>
#include "am.h"
#include "mocks.h"

class am_test : public testing::Test {

};

TEST_F(am_test, empty) {
    auto callback = am_start(123, 100, 5);
    uint8_t buf[] = {1, 2, 3};
    callback(buf, 3);
    auto data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xface0fb0);
    for (auto &x : data) std::cout << std::to_string(x) << std::endl;
}