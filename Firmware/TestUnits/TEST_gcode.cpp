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
    ASSERT_TRUE(gc1.hasG());
    ASSERT_TRUE(!gc1.hasM());
    ASSERT_EQUALS_V(32, gc1.getCode());
    ASSERT_EQUALS_V(0, gc1.getSubcode());
    ASSERT_EQUALS_V(2, gc1.getNumArgs());
    ASSERT_TRUE(gc1.hasArg('X'));
    ASSERT_TRUE(gc1.hasArg('Y'));
    ASSERT_TRUE(!gc1.hasArg('Z'));
    ASSERT_EQUALS_DELTA_V(1.2, gc1.getArg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc1.getArg('Y'), 0.001);
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

    ASSERT_TRUE(gc2.hasG());
    ASSERT_TRUE(!gc2.hasM());
    ASSERT_EQUALS_V(32, gc2.getCode());
    ASSERT_EQUALS_V(2, gc2.getSubcode());
    ASSERT_EQUALS_V(2, gc2.getNumArgs());
    ASSERT_TRUE(gc2.hasArg('X'));
    ASSERT_TRUE(gc2.hasArg('Y'));
    ASSERT_EQUALS_DELTA_V(1.2, gc2.getArg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc2.getArg('Y'), 0.001);
}

TEST(GCodeTest,copy)
{
    // test equals
    GCode gc3;
    gc3= gc2;
    ASSERT_TRUE(gc3.hasG());
    ASSERT_TRUE(!gc3.hasM());
    ASSERT_EQUALS_V(32, gc3.getCode());
    ASSERT_EQUALS_V(2, gc3.getSubcode());
    ASSERT_EQUALS_V(2, gc3.getNumArgs());
    ASSERT_TRUE(gc3.hasArg('X'));
    ASSERT_TRUE(gc3.hasArg('Y'));
    ASSERT_EQUALS_DELTA_V(1.2, gc3.getArg('X'), 0.001);
    ASSERT_EQUALS_DELTA_V(2.3, gc3.getArg('Y'), 0.001);

    // test copy ctor
    GCode gc4(gc2);
	ASSERT_TRUE(gc4.hasG());
	ASSERT_TRUE(!gc4.hasM());
	ASSERT_EQUALS_V(32, gc4.getCode());
	ASSERT_EQUALS_V(2, gc4.getSubcode());
	ASSERT_EQUALS_V(2, gc4.getNumArgs());
	ASSERT_TRUE(gc4.hasArg('X'));
	ASSERT_TRUE(gc4.hasArg('Y'));
	ASSERT_EQUALS_DELTA_V(1.2, gc4.getArg('X'), 0.001);
	ASSERT_EQUALS_DELTA_V(2.3, gc4.getArg('Y'), 0.001);
}
