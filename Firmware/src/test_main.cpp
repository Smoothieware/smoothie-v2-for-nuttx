#include "GCodeProcessor.h"
#include "Dispatcher.h"
#include "GCode.h"
#include "OutputStream.h"

#include <stdio.h>

#include <nuttx/config.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

extern void TEST_main();

bool cb1= false;
bool cb2= false;
bool cb3= false;
extern "C"
{
    int smoothie_main(int argc, char *argv[])
    {
        // If C++ initialization for static constructors is supported, then do
        // that first


        up_cxxinitialize();

        OutputStream o;
        GCodeProcessor gp;

        auto fnc1= [](GCode& gc) { printf("G1 handler: "); gc.dump(); cb1= true; return true; };
        auto fnc2= [](GCode& gc) { printf("M1 handler: "); gc.dump(); cb2= true; return true; };
        auto fnc3= [](GCode& gc) { printf("Second G1 handler: "); gc.dump(); cb3= true; return true; };


        printf( "Parse GCodes\n");

            GCodeProcessor::GCodes_t gcodes;
            bool ok= gp.parse("G1 X1 Y2 M1 G4 S10", gcodes);
            if(!ok) printf("parse failed\n");
            if(gcodes.size() != 3 ) printf("Incorrect gcodes size\n");

            cb1= false;
            cb2= false;
            cb3= false;

        printf( "Dispatch GCodes\n");

            THEDISPATCHER.clear_handlers();
            THEDISPATCHER.add_handler(Dispatcher::GCODE_HANDLER, 1, fnc1);
            THEDISPATCHER.add_handler(Dispatcher::MCODE_HANDLER, 1, fnc2);
            THEDISPATCHER.add_handler(Dispatcher::GCODE_HANDLER, 1, fnc3);

            if(!THEDISPATCHER.dispatch(gcodes[0])) printf("dispatch 0 failed\n");
            if(!THEDISPATCHER.dispatch(gcodes[1])) printf("dispatch 1 failed\n");
            if(THEDISPATCHER.dispatch(gcodes[2])) printf("dispatch 2 didn't fail\n");

            if(!cb1) printf("callback 1 failed\n");
            if(!cb2) printf("callback 2 failed\n");
            if(!cb3) printf("callback 3 failed\n");

      return 0;
    }
}

// int main()
// {
// 	smoothie_main(0, 0);
// }
