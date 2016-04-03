#include <gtest/gtest.h>
#include "ad.h"
#include "mocks.h"

using namespace pebble;

class ad_test : public testing::Test {
protected:
    static uint8_t *buffer;
    static uint16_t size;
public:
    static void ad_callback(const uint8_t *b, const uint16_t s, const double, const uint16_t);

    virtual ~ad_test();
};

ad_test::~ad_test() {
   if (buffer != nullptr) free(buffer);
}

void ad_test::ad_callback(const uint8_t *b, const uint16_t s, const double, const uint16_t) {
    if (buffer != nullptr) free(buffer);
    buffer = (uint8_t *)malloc(s);
    memcpy(buffer, b, s);
    size = s;
}

uint8_t *ad_test::buffer;
uint16_t ad_test::size;

TEST_F(ad_test, overflows) {
    std::vector<AccelRawData> mock_data;
    AccelRawData a = { .x = 1000, .y = 5000, .z = -5000 };
    for (int i = 0; i < 2; i++) mock_data.push_back(a);

    ad_start(ad_test::ad_callback, AD_SAMPLING_50HZ, 1000);
    for (int i = 0; i < AD_BUFFER_SIZE / 2 / sizeof(threed_data); i++) *mocks::accel_service() << mock_data;

    ASSERT_TRUE(ad_test::buffer != nullptr);
    threed_data *data = reinterpret_cast<threed_data *>(ad_test::buffer);
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(data[i].x_val, 1000);
        EXPECT_EQ(data[i].y_val, 5000);
        EXPECT_EQ(data[i].z_val, -5000);
    }

    ad_stop();

}
