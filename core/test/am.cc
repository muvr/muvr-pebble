#include <gtest/gtest.h>
#include "am.h"
#include "ad.h"
#include "mocks.h"

class am_test : public testing::Test {
protected:

    void bytes_equal(const std::vector<uint8_t> &actual, const std::initializer_list<uint8_t> &expected) {
        cout_bytes(actual);

        EXPECT_EQ(actual.size(), expected.size());
        auto a = actual.begin();
        for (auto &e : expected) {
            EXPECT_EQ(e, *a);
            a++;
        }
    }

    void cout_bytes(const std::vector<uint8_t> &bytes) const {
        for (auto &b : bytes) {
            std::cout << "0x"
            << std::setfill ('0') << std::setw(2)
            << std::hex << static_cast<int>(b) << ", ";
        }
        std::cout << std::endl;
    }

    /*
    As an exercise in obscene C++, I leave the variadic template functions here
    ---
    template <typename T>
    void bytes_equal(const typename std::vector<T>::iterator &start,
                     const typename std::vector<T>::iterator &end, T t) {
        EXPECT_EQ(t, *start);
    }

    template<typename T, typename... Args>
    void bytes_equal(const typename std::vector<T>::iterator &start,
                     const typename std::vector<T>::iterator &end, T t, Args... args) {
        if (start == end) GTEST_FAIL() << "Too few actual values";
        EXPECT_EQ(t, *start);
        bytes_equal(start + 1, end, args...);
    }
    */

    virtual void SetUp() {
        pebble::mocks::reset();
    }
};

TEST_F(am_test, trivial) {
    auto callback = am_start(123, 100, 2);
    uint8_t buf[] = {1, 1, 2, 2, 3, 3};
    callback(buf, 6, 0, 0);
    auto data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xad000000);
    bytes_equal(data, { 0x61, 0x64, 0x01, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03 });
    am_stop();
    data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xdead0000);
    bytes_equal(data, { 0x61, 0x64, 0x01, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x00 });
}

TEST_F(am_test, accelerometer_data) {
    #define COUNT 50

    threed_data accelerometer[COUNT];
    for (int i = 0; i < COUNT; i++) {
        accelerometer[i].x_val = 1;
        accelerometer[i].y_val = 1;
        accelerometer[i].z_val = 1;
    }

    auto callback = am_start(123, 50, 6);
    callback(reinterpret_cast<uint8_t *>(accelerometer), sizeof(accelerometer), 0, 1000);
    auto data = pebble::mocks::app_messages()->last_dict().get<std::vector<uint8_t>>(0xad000000);
    EXPECT_EQ(data.size(), COUNT * 6 + sizeof(header));
    cout_bytes(data);
}

/*

This is only applicable with queueing AM

TEST_F(am_test, timeout_on_send) {
    auto callback = am_start(123, 100, 1);
    uint8_t buf1[] = { 99, 100, 101};
    uint8_t buf2[] = {199, 200, 201};

    pebble::mocks::app_messages()->set_outbox_send_result(APP_MSG_INTERNAL_ERROR);
    callback(buf1, 3, 0, 0);

    pebble::mocks::app_messages()->set_outbox_send_result(APP_MSG_OK);
    callback(buf2, 3, 0, 0);

    auto dicts = pebble::mocks::app_messages()->dicts();
    auto data1 = dicts[0].get<std::vector<uint8_t>>(0xface0fb0);
    auto data2 = dicts[1].get<std::vector<uint8_t>>(0xface0fb0);
    bytes_equal<uint8_t>(data1, {123, 3, 100, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 99, 100, 101});
    bytes_equal<uint8_t>(data2, {123, 3, 100, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 199, 200, 201});

    am_stop();
}
*/