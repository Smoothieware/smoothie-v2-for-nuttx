#include "PlannerQueue.h"
#include "Block.h"

#include "../easyunit/test.h"

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

TEST(PlannerQueue,basic)
{
    PlannerQueue rb(10);
    ASSERT_TRUE(rb.empty());
    ASSERT_FALSE(rb.full());

    // this always returens something
    ASSERT_TRUE(rb.get_head() != nullptr);

    // add one entry
    ASSERT_TRUE(rb.queue_head());
    ASSERT_FALSE(rb.empty());
    ASSERT_FALSE(rb.full());

    // add 8 mor eentries
    for (int i = 2; i <= 9; ++i) {
        ASSERT_TRUE(rb.queue_head());
        if(i < 9) ASSERT_FALSE(rb.full())
        else ASSERT_TRUE(rb.full());
    }

    ASSERT_FALSE(rb.empty());
    ASSERT_FALSE(rb.queue_head());
    ASSERT_TRUE(rb.full());

    // remove 8 entries
    for (uint8_t i = 1; i <= 8; ++i) {
        ASSERT_FALSE(rb.empty());
        Block *b= rb.get_tail();
        ASSERT_TRUE(b != nullptr);
        rb.release_tail();
        ASSERT_FALSE(rb.full());
    }

    // one left
    ASSERT_FALSE(rb.empty());
    ASSERT_TRUE(rb.get_tail() != nullptr);

    // release it
    rb.release_tail();

    // none left
    ASSERT_TRUE(rb.empty());
    ASSERT_TRUE(rb.get_tail() == nullptr);
}
