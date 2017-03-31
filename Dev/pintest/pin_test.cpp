//***************************************************************************
// examples/helloxx/helloxx_main.cxx
//
//   Copyright (C) 2009, 2011-2013 Gregory Nutt. All rights reserved.
//   Author: Gregory Nutt <gnutt@nuttx.org>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in
//    the documentation and/or other materials provided with the
//    distribution.
// 3. Neither the name NuttX nor the names of its contributors may be
//    used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
// AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//***************************************************************************

//***************************************************************************
// Included Files
//***************************************************************************

#include <nuttx/config.h>

#include <cstdio>
#include <debug.h>

#include <nuttx/init.h>
#include <nuttx/arch.h>

//***************************************************************************
// Definitions
//***************************************************************************
// Configuration ************************************************************
// C++ initialization requires CXX initializer support

#if !defined(CONFIG_HAVE_CXX) || !defined(CONFIG_HAVE_CXXINITIALIZE)
#  undef CONFIG_SMOOTHIEWARE_CXXINITIALIZE
#endif

// Debug ********************************************************************
// Non-standard debug that may be enabled just for testing the constructors

#ifndef CONFIG_DEBUG_FEATURES
#  undef CONFIG_DEBUG_CXX
#endif

#ifdef CONFIG_DEBUG_CXX
#  define cxxinfo     _info
#else
#  define cxxinfo(x...)
#endif

/****************************************************************************
 * Name: helloxx_main
 ****************************************************************************/
#include "Pin.h"
#include <array>

extern "C"
{
    int main(int argc, char *argv[])
    {
        // If C++ initialization for static constructors is supported, then do
        // that first

#ifdef CONFIG_SMOOTHIEWARE_CXXINITIALIZE
        up_cxxinitialize();
#endif

        int cnt = 0;
        printf("defining pins...\n");
        Pin myleds[]= {
            Pin("GPIO1[0]"),
            Pin("GPIO3[3]"),
            Pin("GPIO3[4]"),
            Pin("GPIO2[6]"),
            Pin("GPIO5[12]"),
            Pin("GPIO5[13]"),
            Pin("P4_10") // Pin("GPIO5[14]")
        };

        Pin button("GPIO0_7!");
        button.as_input();
        if(!button.connected()) {
            printf("Button was invalid\n");
        }

        printf("set as outputs... \n");
        for(auto& p : myleds) {
            cnt++;
            if(p.as_output() == nullptr) {
                printf("Failed to allocate pin %d\n", cnt);
            }else{
                p.set(false);
                printf("Set pin %d, %04X\n", cnt, p.get_gpiocfg());
            }
        }

        printf("Running...\n");
        cnt= 0;
        while(button.get() == 0){
            uint8_t m= 1;
            for(auto& p : myleds) {
                p.set(cnt & m);
                m <<= 1;
                cnt++;
            }
            usleep(50000);
        }
        printf("Done\n");

        return 0;
    }
}
