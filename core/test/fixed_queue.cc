#include <gtest/gtest.h>
#include "fixed_queue.nh"

class fixed_queue_test : public testing::Test {
protected:
    uint8_t *make_buffer(const uint8_t element) {
        auto b = new uint8_t[1];
        b[0] = element;
        return b;
    };
};

TEST_F(fixed_queue_test, simple) {
    queue_t *q = queue_create(8);
    queue_add(q, 0xface0fb0, make_buffer(1), 1, 0);
    queue_add(q, 0xface0fb0, make_buffer(2), 1, 0);
    queue_add(q, 0xface0fb0, make_buffer(3), 1, 0);

    EXPECT_EQ(3, queue_length(q));
    uint8_t buffer[1];
    uint32_t key;
    uint64_t timestamp;
    EXPECT_EQ(1, queue_peek(q, &key, buffer, 1, &timestamp)); EXPECT_EQ(1, buffer[0]); EXPECT_EQ(0xface0fb0, key); queue_tail(q);
    EXPECT_EQ(1, queue_peek(q, &key, buffer, 1, &timestamp)); EXPECT_EQ(2, buffer[0]); EXPECT_EQ(0xface0fb0, key); queue_tail(q);
    EXPECT_EQ(1, queue_peek(q, &key, buffer, 1, &timestamp)); EXPECT_EQ(3, buffer[0]); EXPECT_EQ(0xface0fb0, key); queue_tail(q);
    EXPECT_EQ(0, queue_peek(q, &key, buffer, 1, &timestamp));

    queue_destroy(&q);
    EXPECT_EQ(nullptr, q);
};

TEST_F(fixed_queue_test, edge) {
    queue_t *q = queue_create(8);

    // cannot add empty buffer
    EXPECT_EQ(0, queue_add(q, 0, nullptr, 0, 0));

    // OK to add one
    EXPECT_EQ(1, queue_add(q, 0xface0fb0, make_buffer(1), 1, 0));

    // cannot peek into nullptr buffer
    EXPECT_EQ(1, queue_peek(q, nullptr, nullptr, 0, nullptr));
    // cannot peek into too small buffer
    uint8_t buffer[1];
    uint32_t key;
    uint64_t timestamp;
    EXPECT_EQ(1, queue_peek(q, &key, buffer, 0, &timestamp));

    // element still in the queue
    EXPECT_EQ(1, queue_length(q));

    queue_destroy(&q);
    EXPECT_EQ(nullptr, q);
}
