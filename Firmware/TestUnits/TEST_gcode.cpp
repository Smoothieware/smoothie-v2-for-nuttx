#include "GCode.h"
#include "GCodeProcessor.h"

#include "../easyunit/test.h"

TEST(GCodeTest,basic)
{
    GCodeProcessor gp;
    GCodeProcessor::GCodes_t gca;

    const char *g1("G32 X1.2 Y2.3");
    bool ok= gp.parse(g1, gca);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(1, gca.size());
    GCode gc1= gca[0];
    ASSERT_TRUE(gc1.has_g());
    ASSERT_TRUE(!gc1.has_m());
    ASSERT_EQUALS_V(32, gc1.get_code());
    ASSERT_EQUALS_V(0, gc1.get_subcode());
    ASSERT_EQUALS_V(2, gc1.get_num_args());
    ASSERT_TRUE(gc1.has_arg('X'));
    ASSERT_TRUE(gc1.has_arg('Y'));
    ASSERT_TRUE(!gc1.has_arg('Z'));
    ASSERT_EQUALS_DELTA_V(1.2, gc1.get_arg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc1.get_arg('Y'), 0.001);
}

GCode gc2;

TEST(GCodeTest,subcode)
{
    GCodeProcessor gp;
    GCodeProcessor::GCodes_t gca;
    const char *g2("G32.2 X1.2 Y2.3");
    bool ok= gp.parse(g2, gca);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(1, gca.size());
    gc2= gca[0];

    ASSERT_TRUE(gc2.has_g());
    ASSERT_TRUE(!gc2.has_m());
    ASSERT_EQUALS_V(32, gc2.get_code());
    ASSERT_EQUALS_V(2, gc2.get_subcode());
    ASSERT_EQUALS_V(2, gc2.get_num_args());
    ASSERT_TRUE(gc2.has_arg('X'));
    ASSERT_TRUE(gc2.has_arg('Y'));
    ASSERT_EQUALS_DELTA_V(1.2, gc2.get_arg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc2.get_arg('Y'), 0.001);
}

TEST(GCodeTest,copy)
{
    // test equals
    GCode gc3;
    gc3= gc2;
    ASSERT_TRUE(gc3.has_g());
    ASSERT_TRUE(!gc3.has_m());
    ASSERT_EQUALS_V(32, gc3.get_code());
    ASSERT_EQUALS_V(2, gc3.get_subcode());
    ASSERT_EQUALS_V(2, gc3.get_num_args());
    ASSERT_TRUE(gc3.has_arg('X'));
    ASSERT_TRUE(gc3.has_arg('Y'));
    ASSERT_EQUALS_DELTA_V(1.2, gc3.get_arg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc3.get_arg('Y'), 0.001);

    // test copy ctor
    GCode gc4(gc2);
	ASSERT_TRUE(gc4.has_g());
	ASSERT_TRUE(!gc4.has_m());
	ASSERT_EQUALS_V(32, gc4.get_code());
	ASSERT_EQUALS_V(2, gc4.get_subcode());
	ASSERT_EQUALS_V(2, gc4.get_num_args());
	ASSERT_TRUE(gc4.has_arg('X'));
	ASSERT_TRUE(gc4.has_arg('Y'));
	ASSERT_EQUALS_DELTA_V(1.2, gc4.get_arg('X'), 0.001);
	ASSERT_EQUALS_DELTA_V(2.3, gc4.get_arg('Y'), 0.001);
}
