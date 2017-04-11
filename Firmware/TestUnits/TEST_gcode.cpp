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

TEST(GCodeTest, Multiple_commands_on_line_no_spaces) {
    GCodeProcessor gp;
    const char *gc= "M123X1Y2G1X10Y20Z0.634";
    GCodeProcessor::GCodes_t gcodes;
    bool ok= gp.parse(gc, gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V( 2, gcodes.size());
    auto a= gcodes[0];
    ASSERT_TRUE(a.has_m());
    ASSERT_EQUALS_V(123, a.get_code());
    ASSERT_TRUE(a.has_arg('X')); ASSERT_EQUALS_V(1, a.get_arg('X'));
    ASSERT_TRUE(a.has_arg('Y')); ASSERT_EQUALS_V(2, a.get_arg('Y'));
    auto b= gcodes[1];
    ASSERT_TRUE(b.has_g());
    ASSERT_EQUALS_V(1, b.get_code());
    ASSERT_TRUE(b.has_arg('X')); ASSERT_EQUALS_V(10, b.get_arg('X'));
    ASSERT_TRUE(b.has_arg('Y')); ASSERT_EQUALS_V(20, b.get_arg('Y'));
    ASSERT_TRUE(b.has_arg('Z')); ASSERT_EQUALS_V(0.634f, b.get_arg('Z'));
}

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

TEST(GCodeTest, Modal_G1_and_comments) {
    GCodeProcessor gp;
    GCodeProcessor::GCodes_t gcodes;
    bool ok= gp.parse("G1 X0", gcodes);
    ASSERT_TRUE(ok);
    const char *gc= "( this is a comment )X100Y200 ; G23 X0";
    gcodes.clear();
    ok= gp.parse(gc, gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(1, gcodes.size());
    auto a = gcodes[0];
    ASSERT_TRUE(a.has_g());
    ASSERT_EQUALS_V(1, a.get_code());
    ASSERT_TRUE(a.has_arg('X'));
    ASSERT_TRUE(a.has_arg('Y'));
    ASSERT_FALSE(a.has_arg('Z'));
}

TEST(GCodeTest, Line_numbers_and_checksums) {
    GCodeProcessor gp;
    GCodeProcessor::GCodes_t gcodes;
    bool ok= gp.parse("N10 G1 X0", gcodes);
    ASSERT_FALSE(ok);
    ASSERT_TRUE(gcodes.empty());

    gcodes.clear();
    ok= gp.parse("N10 M110*123", gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(10, gp.get_line_number());
    ASSERT_TRUE(gcodes.empty());

    // Bad line number
    gcodes.clear();
    ok= gp.parse("N95 G1 X-4.992 Y-14.792 F12000.000*97", gcodes);
    ASSERT_FALSE(ok);
    ASSERT_TRUE(gcodes.empty());

    gcodes.clear();
    ok= gp.parse("N94 M110*123", gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(94, gp.get_line_number());
    ok= gp.parse("N95 G1 X-4.992 Y-14.792 F12000.000*97", gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(1, gcodes.size());

    // Bad checksum
    gcodes.clear();
    ok= gp.parse("N94 M110*123", gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(94, gp.get_line_number());
    ok= gp.parse("N95 G1 X-4.992 Y-14.792 F12000.000*98", gcodes);
    ASSERT_FALSE(ok);
    ASSERT_TRUE(gcodes.empty());
}

