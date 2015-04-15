#include <gtest/gtest.h>
#include "fixed_queue.h"

class fixed_queue_test : public testing::Test {
protected:
    uint8_t *make_buffer(const uint8_t element) {
        auto b = new uint8_t[1];
        b[0] = element;
        return b;
    };
};

TEST_F(fixed_queue_test, simple) {
    queue_t *q = queue_create();
    queue_add(q, make_buffer(1), 1);
    queue_add(q, make_buffer(2), 1);
    queue_add(q, make_buffer(3), 1);

    EXPECT_EQ(3, queue_length(q));
    uint8_t buffer[1];
    EXPECT_EQ(1, queue_peek(q, buffer, 1)); EXPECT_EQ(1, buffer[0]); queue_tail(q);
    EXPECT_EQ(1, queue_peek(q, buffer, 1)); EXPECT_EQ(2, buffer[0]); queue_tail(q);
    EXPECT_EQ(1, queue_peek(q, buffer, 1)); EXPECT_EQ(3, buffer[0]); queue_tail(q);
    EXPECT_EQ(0, queue_peek(q, buffer, 1));

    queue_destroy(&q);
    EXPECT_EQ(nullptr, q);
};

TEST_F(fixed_queue_test, edge) {
    queue_t *q = queue_create();

    // cannot add empty buffer
    EXPECT_EQ(0, queue_add(q, nullptr, 0));

    // OK to add one
    EXPECT_EQ(1, queue_add(q, make_buffer(1), 1));

    // cannot peek into nullptr buffer
    EXPECT_EQ(1, queue_peek(q, nullptr, 0));
    // cannot peek into too small buffer
    uint8_t buffer[1];
    EXPECT_EQ(1, queue_peek(q, buffer, 0));

    // element still in the queue
    EXPECT_EQ(1, queue_length(q));

    queue_destroy(&q);
    EXPECT_EQ(nullptr, q);
}
