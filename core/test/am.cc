#include <gtest/gtest.h>
#include "am.h"
#include "mocks.h"

class am_test : public testing::Test {

};

TEST_F(am_test, trivial) {
    auto callback = am_start(123, 100, 1);
    uint8_t buf[] = {1, 2, 3};
    callback(buf, 3);
    auto data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xface0fb0);

    EXPECT_EQ(123, data[0]);
    EXPECT_EQ(3,   data[1]);
    EXPECT_EQ(100, data[2]);
    EXPECT_EQ(1,   data[3]);
    EXPECT_EQ(0,   data[4]);

    EXPECT_EQ(1,   data[5]);
    EXPECT_EQ(2,   data[6]);
    EXPECT_EQ(3,   data[7]);

    am_stop();
}

TEST_F(am_test, timeout_on_send) {
    auto callback = am_start(123, 100, 1);
    uint8_t buf[] = {1, 2, 3};

    pebble::mocks::app_messages()->set_outbox_send_result(APP_MSG_INTERNAL_ERROR);
    callback(buf, 3);

    pebble::mocks::app_messages()->set_outbox_send_result(APP_MSG_OK);
    callback(buf, 3);

    auto data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xface0fb0);
    for (auto &x : data) std::cout << std::to_string(x) << std::endl;

    am_stop();
}