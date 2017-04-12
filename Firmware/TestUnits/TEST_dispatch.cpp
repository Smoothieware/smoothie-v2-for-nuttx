#include "GCode.h"
#include "GCodeProcessor.h"
#include "Dispatcher.h"

#include "../easyunit/test.h"

#define ASSERT_FALSE(x) ASSERT_TRUE(!(x))

DECLARE(Dispatcher)

bool cb1;
bool cb2;
bool cb3;
GCodeProcessor gp;
GCodeProcessor::GCodes_t gcodes;
Dispatcher::Handlers_t::iterator h3;
GCode::Args_t args;
END_DECLARE

SETUP(Dispatcher)
{
    cb1= false;
    cb2= false;
    cb3= false;
    auto fnc1= [this](GCode& gc) { args= gc.get_args(); cb1= true; return true; };
    auto fnc2= [this](GCode& gc) { cb2= true; return true; };
    auto fnc3= [this](GCode& gc) { cb3= true; return true; };
    THEDISPATCHER.clear_handlers();
    THEDISPATCHER.add_handler(Dispatcher::GCODE_HANDLER, 1, fnc1);
    THEDISPATCHER.add_handler(Dispatcher::MCODE_HANDLER, 1, fnc2);
    h3= THEDISPATCHER.add_handler(Dispatcher::GCODE_HANDLER, 1, fnc3);
    gcodes.clear();
    bool ok= gp.parse("G1 X1 Y2 M1 G4 S10", gcodes);
    ASSERT_TRUE(ok);
    ASSERT_EQUALS_V(3, gcodes.size());
    args.clear();
}

TEARDOWN(Dispatcher)
{
    THEDISPATCHER.clear_handlers();
}

TESTF(Dispatcher, check_callbacks)
{
    ASSERT_FALSE(cb1);
    ASSERT_FALSE(cb2);
    ASSERT_FALSE(cb3);

    ASSERT_TRUE(THEDISPATCHER.dispatch(gcodes[0]));
    ASSERT_TRUE( cb1 );
    ASSERT_FALSE(cb2);
    ASSERT_TRUE( cb3 );
    ASSERT_EQUALS_V(2, args.size());
    ASSERT_EQUALS_V(1, args['X']);
    ASSERT_EQUALS_V(2, args['Y']);

    ASSERT_TRUE(THEDISPATCHER.dispatch(gcodes[1]));
    ASSERT_TRUE( cb2 );
    ASSERT_FALSE(THEDISPATCHER.dispatch(gcodes[2]));
}

TESTF(Dispatcher, Remove_second_G1_handler)
{
    ASSERT_FALSE(cb1);
    ASSERT_FALSE(cb2);
    ASSERT_FALSE(cb3);

    THEDISPATCHER.remove_handler(Dispatcher::GCODE_HANDLER, h3);
    ASSERT_TRUE(THEDISPATCHER.dispatch(gcodes[0]));
    ASSERT_TRUE ( cb1 );
    ASSERT_FALSE ( cb3 );
}

TESTF(Dispatcher, one_off_dispatch)
{
    ASSERT_FALSE(cb1);
    ASSERT_TRUE(args.empty());
    THEDISPATCHER.dispatch('G', 1, 'X', 456.0, 'Y', 789.0, 'Z', 123.0, 0);
    ASSERT_TRUE ( cb1 );
    for(auto &i : args) {
        printf("%c: %f\n", i.first, i.second);
    }
    ASSERT_EQUALS_V(3, args.size());
    ASSERT_EQUALS_V(456, args['X']);
    ASSERT_EQUALS_V(789, args['Y']);
    ASSERT_EQUALS_V(123, args['Z']);
}

