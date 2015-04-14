#include <gtest/gtest.h>
#include "ad.h"
#include "instances.h"

using namespace pebble;

class gfs_test : public testing::Test {
protected:
    static uint8_t *buffer;
    static uint16_t size;
public:
    static void gfs_callback(uint8_t *b, uint16_t s);

    virtual ~gfs_test();
};

gfs_test::~gfs_test() {
   if (buffer != nullptr) free(buffer);
}

void gfs_test::gfs_callback(uint8_t *b, uint16_t s) {
    if (buffer != nullptr) free(buffer);
    buffer = (uint8_t *)malloc(s);
    memcpy(buffer, b, s);
    size = s;
}

uint8_t *gfs_test::buffer;
uint16_t gfs_test::size;

TEST_F(gfs_test, Version1) {
    // we have 12000B for our buffer, which means space for 2384 samples
    // the accelerometer is configured to receive multiple of 8 number
    // if AccelRawData values.

    // that is, we must push 376 * 8 packets of accel data
    std::vector<AccelRawData> mock_data;
    AccelRawData a = { .x = 1000, .y = 5000, .z = -5000 };
    for (int i = 0; i < 2; i++) mock_data.push_back(a);

    ad_start(gfs_test::gfs_callback, GFS_SAMPLING_100HZ);
    for (int i = 0; i < 126; i++) instances::accel_service << mock_data;

    ASSERT_TRUE(gfs_test::buffer != nullptr);
    threed_data *data = reinterpret_cast<threed_data *>(gfs_test::buffer);
    for (int i = 0; i < 126; i++) {
        EXPECT_EQ(data[i].x_val, 1000);
        EXPECT_EQ(data[i].y_val, 4095);
        EXPECT_EQ(data[i].z_val, -4095);
    }

    ad_stop();

}

