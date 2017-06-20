/*
      this file is part of smoothie (http://smoothieware.org/). the motion control part is heavily based on grbl (https://github.com/simen/grbl).
      smoothie is free software: you can redistribute it and/or modify it under the terms of the gnu general public license as published by the free software foundation, either version 3 of the license, or (at your option) any later version.
      smoothie is distributed in the hope that it will be useful, but without any warranty; without even the implied warranty of merchantability or fitness for a particular purpose. see the gnu general public license for more details.
      you should have received a copy of the gnu general public license along with smoothie. if not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#if 1
#include "Pin.h"
//#include "Pwm.h"

#include <string>

class GCode;
class StreamOutput;

// namespace mbed {
//     class PwmOut;
// }

class Switch : public Module {
    public:
        Switch(const char *name);
        static bool load_switches();

        bool configure();
        void on_halt(bool);
        bool request(const char *key, void *value) ;

        enum OUTPUT_TYPE {NONE, SIGMADELTA, DIGITAL, HWPWM};

    private:
        void pinpoll_tick(void);
        void flip();
        void send_gcode(std::string& msg);
        bool match_input_on_gcode(const GCode& gcode) const;
        bool match_input_off_gcode(const GCode& gcode) const;

        Pin       input_pin;
        float     switch_value;
        OUTPUT_TYPE output_type;
        union {
            Pin          *digital_pin;
            //Pwm          *sigmadelta_pin;
            //mbed::PwmOut *pwm_pin;
        };
        std::string    output_on_command;
        std::string    output_off_command;
        uint16_t  input_pin_behavior;
        uint16_t  input_on_command_code;
        uint16_t  input_off_command_code;
        char      input_on_command_letter;
        char      input_off_command_letter;
        uint8_t   subcode;
        bool      switch_changed;
        bool      input_pin_state;
        bool      switch_state;
        bool      ignore_on_halt;
        uint8_t   failsafe;
};

