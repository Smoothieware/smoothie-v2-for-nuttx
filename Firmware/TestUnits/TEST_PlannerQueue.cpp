#include "PlannerQueue.h"
#include "Block.h"

#include "../easyunit/test.h"

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

TEST(PlannerQueue,basic)
{
    PlannerQueue rb(10);
    ASSERT_TRUE(rb.empty());
    ASSERT_FALSE(rb.full());

    // this always returns something
    ASSERT_TRUE(rb.get_head() != nullptr);

    // add one entry
    ASSERT_TRUE(rb.queue_head());
    ASSERT_FALSE(rb.empty());
    ASSERT_FALSE(rb.full());

    // add 8 more entries
    for (int i = 2; i <= 9; ++i) {
        ASSERT_TRUE(rb.queue_head());
        if(i < 9) ASSERT_FALSE(rb.full())
        else ASSERT_TRUE(rb.full());
    }

    ASSERT_FALSE(rb.empty());
    ASSERT_FALSE(rb.queue_head());
    ASSERT_TRUE(rb.full());

    // remove 8 entries
    for (int i = 1; i <= 8; ++i) {
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

TEST(PlannerQueue,iteration)
{
    PlannerQueue rb(10);
	ASSERT_TRUE(rb.empty());
    ASSERT_FALSE(rb.full());

    // add 4 entries
    for (int i = 1; i <= 4; ++i) {
        Block *b= rb.get_head();
        b->steps_event_count= i;
        ASSERT_TRUE(rb.queue_head());
    }

	ASSERT_FALSE(rb.empty());

    rb.start_iteration();
    ASSERT_TRUE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

    // 1
    Block *b= rb.tailward_get();
    ASSERT_EQUALS_V(4, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

    // 2
    b= rb.tailward_get();
    ASSERT_EQUALS_V(3, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

    // 3
    b= rb.tailward_get();
    ASSERT_EQUALS_V(2, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

    // 4
    b= rb.tailward_get();
    ASSERT_EQUALS_V(1, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_TRUE(rb.is_at_tail());

	// 3
    b= rb.headward_get();
    ASSERT_EQUALS_V(2, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

	// 2
    b= rb.headward_get();
    ASSERT_EQUALS_V(3, b->steps_event_count);
    ASSERT_FALSE(rb.is_at_head());
    ASSERT_FALSE(rb.is_at_tail());

	// 1
	b= rb.headward_get();
    ASSERT_EQUALS_V(4, b->steps_event_count);
	ASSERT_FALSE(rb.is_at_head());
	ASSERT_FALSE(rb.is_at_tail());

	// back at head
	b= rb.headward_get();
	ASSERT_TRUE(rb.is_at_head());
}
