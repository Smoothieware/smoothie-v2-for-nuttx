//***************************************************************************
// Included Files
//***************************************************************************

#include <nuttx/config.h>

#include <stdio.h>
#include <debug.h>

#include <nuttx/init.h>
#include <nuttx/arch.h>


/****************************************************************************
 * Name: helloxx_main
 ****************************************************************************/
#include "Pin.h"
#include <array>

float test_floats(float a, float b)
{
	return a * b;
}

// #include <sstream>
// void test_sstream()
// {
// 	std::ostringstream oss;
// 	oss << "Hello\n";
// 	std::string result= oss.str();
// 	printf("test sstream: %s", result.c_str());
// }

#include <vector>
void test_vector()
{
	std::vector<int> v;
	v.push_back(1);
	int i= v.at(0);
	printf("vector size: %d, value: %d\n", v.size(), i);
}

#include <string>
void test_string()
{
 	std::string s;
	s= "abcd";
	char c= s.at(1);
	s.erase(1,2);
 	printf("string: %s, %c\n", s.c_str(), c);
}

#if 0
#include <sstream>
void test_sstream()
{
	printf("Testing sstream...\n");
	std::ostringstream oss;
	oss << "Hello World!";
	printf("oss = %s\n", oss.str().c_str());
}
#endif

void do_crash()
{
	int *p = (int *)0xCCCCCCCC;
	int x= *p;

}
extern "C" int smoothie_main(int argc, char *argv[])
{
       // up_cxxinitialize();

/*
		float a= 10.123F;
		float b= 20.456F;
		float x= test_floats(a, b);
		printf("float = %f\n", x); // NOTE thisis broken in NUTTX
		printf("float = %10.8f\n", x); // this works
*/
		//test_sstream();

        int cnt = 0;
        printf("defining pins...\n");
        #if 0
        Pin myleds[]= {
            Pin("GPIO1[0]"),
            Pin("GPIO3[3]"),
            Pin("GPIO3[4]"),
            Pin("GPIO2[6]"),
            Pin("GPIO5[12]"),
            Pin("GPIO5[13]"),
            Pin("P4_10") // Pin("GPIO5[14]")
        };
        #else
        Pin myleds[]= {
            //Pin("GPIO2[10]"),
            //Pin("GPIO2[9]"),
            Pin("GPIO6[13]"),
            Pin("GPIO6[12]")
        };
        #endif

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
            }
            cnt++;
            usleep(500000);
        }
        printf("Done\n");

		//do_crash();

        return 0;
    }

