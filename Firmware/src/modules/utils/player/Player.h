#pragma once

#include "Module.h"

#include <string>
#include <map>
#include <vector>

class OutputStream;
class GCode;

class Player : public Module {
    public:
        Player();

        bool configure(ConfigReader&);
        void on_halt(bool flg);

    private:
        bool handle_gcode(GCode& gcode, OutputStream& os);
        bool play_command( std::string& parameters, OutputStream& os );
        bool progress_command( std::string& parameters, OutputStream& os );
        bool abort_command( std::string& parameters, OutputStream& os );
        bool suspend_command( std::string& parameters, OutputStream& os );
        bool resume_command( std::string& parameters, OutputStream& os );
        std::string extract_options(std::string& args);
        void suspend_part2();

        std::string filename;
        std::string after_suspend_gcode;
        std::string before_resume_gcode;
        std::string on_boot_gcode;
        OutputStream *current_stream;
        OutputStream *reply_stream;

        FILE* current_file_handler;
        long file_size;
        unsigned long played_cnt;
        unsigned long elapsed_secs;
        float saved_position[3]; // only saves XYZ
        std::map<uint16_t, float> saved_temperatures;
        struct {
            bool on_boot_gcode_enable:1;
            bool booted:1;
            bool playing_file:1;
            bool suspended:1;
            bool was_playing_file:1;
            bool leave_heaters_on:1;
            bool override_leave_heaters_on:1;
            bool halted:1;
            uint8_t suspend_loops:4;
        };
};
