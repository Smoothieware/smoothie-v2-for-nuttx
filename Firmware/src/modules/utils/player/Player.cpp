#include "Player.h"

#include "Robot.h"
#include "OutputStream.h"
#include "GCode.h"
#include "ConfigReader.h"
#include "Dispatcher.h"
#include "Conveyor.h"

#include <cstddef>
#include <cmath>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>

#define on_boot_gcode_key "on_boot_gcode"
#define on_boot_gcode_enable_key "on_boot_gcode_enable"
#define after_suspend_gcode_key "after_suspend_gcode"
#define before_resume_gcode_key "before_resume_gcode"
#define leave_heaters_on_suspend_key "leave_heaters_on_suspend"

#define HELP(m) if(params == "-h") { os.printf("%s\n", m); return true; }

Player::Player() : Module("player")
{
    this->playing_file = false;
    this->current_file_handler = nullptr;
    this->booted = false;
    this->elapsed_secs = 0;
    this->reply_stream = nullptr;
    this->suspended = false;
    this->suspend_loops = 0;
    halted = false;
}

bool Player::configure(ConfigReader& cr)
{
    ConfigReader::section_map_t m;
    if(!cr.get_section("player", m)) {
        printf("WARNING:configure-player: no player section found, defaults used\n");
    }

    this->on_boot_gcode = cr.get_string(m, on_boot_gcode_key, "/sd/on_boot.gcode");
    this->on_boot_gcode_enable = cr.get_bool(m, on_boot_gcode_enable_key, true);

    this->leave_heaters_on = cr.get_bool(m, leave_heaters_on_suspend_key, false);
    this->after_suspend_gcode = cr.get_string(m, after_suspend_gcode_key, "");
    this->before_resume_gcode = cr.get_string(m, before_resume_gcode_key, "");
    std::replace( this->after_suspend_gcode.begin(), this->after_suspend_gcode.end(), '_', ' '); // replace _ with space
    std::replace( this->before_resume_gcode.begin(), this->before_resume_gcode.end(), '_', ' '); // replace _ with space

    // g/m code handlers
    using std::placeholders::_1;
    using std::placeholders::_2;

    THEDISPATCHER->add_handler(Dispatcher::GCODE_HANDLER, 28, std::bind(&Player::handle_gcode, this, _1, _2));

    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 21, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 24, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 25, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 26, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 27, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 600, std::bind(&Player::handle_gcode, this, _1, _2));
    THEDISPATCHER->add_handler(Dispatcher::MCODE_HANDLER, 601, std::bind(&Player::handle_gcode, this, _1, _2));

    // These are special as they violate g code specs and pass a string parameter (filename)
    THEDISPATCHER->add_handler("m23", std::bind(&Player::handle_m23, this, _1, _2));
    THEDISPATCHER->add_handler("m32", std::bind(&Player::handle_m32, this, _1, _2));

    // command handlers
    THEDISPATCHER->add_handler( "play", std::bind( &Player::play_command, this, _1, _2) );
    THEDISPATCHER->add_handler( "progress", std::bind( &Player::progress_command, this, _1, _2) );
    THEDISPATCHER->add_handler( "abort", std::bind( &Player::abort_command, this, _1, _2) );
    THEDISPATCHER->add_handler( "suspend", std::bind( &Player::suspend_command, this, _1, _2) );
    THEDISPATCHER->add_handler( "resume", std::bind( &Player::resume_command, this, _1, _2) );

    return true;
}

void Player::on_halt(bool flg)
{
    halted = flg;
    if(flg && this->playing_file ) {
        OutputStream os; // null stream
        std::string cmd;
        abort_command(cmd, os);
    }
}

// TODO implement
// void Player::on_second_tick(void *)
// {
//     if(this->playing_file) this->elapsed_secs++;
// }

// extract any options found on line, terminates args at the space before the first option (-v)
// eg this is a file.gcode -v
//    will return -v and set args to this is a file.gcode
std::string Player::extract_options(std::string& args)
{
    std::string opts;
    size_t pos = args.find(" -");
    if(pos != std::string::npos) {
        opts = args.substr(pos);
        args = args.substr(0, pos);
    }

    return opts;
}

bool Player::handle_m23(std::string& params, OutputStream& os)
{
    HELP("M23 - select file")

    // M23 select file

    // TODO this duplicates play()
    this->filename = "/sd/" + params; // filename is whatever is in params
    this->current_stream = nullptr;

    if(this->current_file_handler != NULL) {
        this->playing_file = false;
        fclose(this->current_file_handler);
    }
    this->current_file_handler = fopen( this->filename.c_str(), "r");

    if(this->current_file_handler == NULL) {
        os.printf("file.open failed: %s\n", this->filename.c_str());
        return true;

    } else {
        // get size of file
        int result = fseek(this->current_file_handler, 0, SEEK_END);
        if (0 != result) {
            this->file_size = 0;
        } else {
            this->file_size = ftell(this->current_file_handler);
            fseek(this->current_file_handler, 0, SEEK_SET);
        }
        os.printf("File opened:%s Size:%ld\n", this->filename.c_str(), this->file_size);
        os.printf("File selected\n");
    }


    this->played_cnt = 0;
    this->elapsed_secs = 0;
    return true;
}

bool Player::handle_m32(std::string& params, OutputStream& os)
{
    HELP("M32 - play file")
    // TODO this duplicates play()
    // M32 select file and start print
    this->filename = "/sd/" + params; // filename is whatever is in params including spaces
    this->current_stream = nullptr;

    if(this->current_file_handler != NULL) {
        this->playing_file = false;
        fclose(this->current_file_handler);
    }

    this->current_file_handler = fopen( this->filename.c_str(), "r");
    if(this->current_file_handler == NULL) {
        os.printf("file.open failed: %s\n", this->filename.c_str());
    } else {
        this->playing_file = true;

        // get size of file
        int result = fseek(this->current_file_handler, 0, SEEK_END);
        if (0 != result) {
            file_size = 0;
        } else {
            file_size = ftell(this->current_file_handler);
            fseek(this->current_file_handler, 0, SEEK_SET);
        }
    }

    this->played_cnt = 0;
    this->elapsed_secs = 0;
    return true;
}

bool Player::handle_gcode(GCode& gcode, OutputStream& os)
{
    //std::string args = get_arguments(gcode.get_command()); // TODO gcode cannot have string arguments how to fix?
    if (gcode.has_m()) {
        switch(gcode.get_code()) {
            case 21: // Dummy code; makes Octoprint happy -- supposed to initialize SD card
                os.printf("SD card ok\n");
                break;

            case 24: // start print
                if (this->current_file_handler != NULL) {
                    this->playing_file = true;
                    // this would be a problem if the stream goes away before the file has finished,
                    // so we attach it to the kernel stream, however network connections from pronterface
                    // do not connect to the kernel streams so won't see this FIXME
                    //this->reply_stream = THEKERNEL->streams; // TODO implement
                }
                break;

            case 25:  // pause print
                this->playing_file = false;
                break;

            case 26: // Reset print. Slightly different than M26 in Marlin and the rest
                if(this->current_file_handler != NULL) {
                    std::string currentfn = this->filename.c_str();
                    unsigned long old_size = this->file_size;

                    // abort the print
                    std::string cmd;
                    abort_command(cmd, os);

                    if(!currentfn.empty()) {
                        // reload the last file opened
                        this->current_file_handler = fopen(currentfn.c_str() , "r");

                        if(this->current_file_handler == NULL) {
                            os.printf("file.open failed: %s\n", currentfn.c_str());
                        } else {
                            this->filename = currentfn;
                            this->file_size = old_size;
                            this->current_stream = nullptr;
                        }
                    }
                } else {
                    os.printf("No file loaded\n");
                }
                break;

            case 27: { // report print progress, in format used by Marlin
                std::string cmd("-b");
                progress_command(cmd, os);
            }
            break;

            case 600: { // suspend print, Not entirely Marlin compliant, M600.1 will leave the heaters on
                std::string cmd((gcode.get_subcode() == 1) ? "h" : "");
                this->suspend_command(cmd, os);
            }
            break;

            case 601: { // resume print
                std::string cmd;
                this->resume_command(cmd, os);
            }
            break;

            default:
                return false;
        }

    } else if(gcode.has_g()) {
        // TODO handle grbl mode
        if(gcode.get_code() == 28 && gcode.get_subcode() == 0) { // homing cancels suspend
            if(this->suspended) {
                // clean up
                this->suspended = false;
                Robot::getInstance()->pop_state();
                this->saved_temperatures.clear();
                this->was_playing_file = false;
                this->suspend_loops = 0;
                os.printf("//Suspend cancelled due to homing cycle\n");
            }
        }
    }

    return true;
}

// Play a gcode file by considering each line as if it was received on the serial console
bool Player::play_command( std::string& params, OutputStream& os )
{
    HELP("play [-v] file")

    // extract any options from the line and terminate the line there
    std::string options = extract_options(params);
    // Get filename which is the entire parameter line upto any options found or entire line
    this->filename = params;

    if(this->playing_file || this->suspended) {
        os.printf("Currently printing, abort print first\n");
        return true;
    }

    if(this->current_file_handler != NULL) { // must have been a paused print
        fclose(this->current_file_handler);
    }

    this->current_file_handler = fopen( this->filename.c_str(), "r");
    if(this->current_file_handler == NULL) {
        os.printf("File not found: %s\n", this->filename.c_str());
        return true;
    }

    os.printf("Playing %s\n", this->filename.c_str());

    this->playing_file = true;

    // Output to the current stream if we were passed the -v ( verbose ) option
    if( options.find_first_of("Vv") == std::string::npos ) {
        this->current_stream = nullptr;
    } else {
        // we send to the kernels stream as it cannot go away
        //this->current_stream = THEKERNEL->streams; // TODO may need to printf
    }

    // get size of file
    struct stat buf;
    if (stat(filename.c_str(), &buf) >= 0) {
        file_size = buf.st_size;
        os.printf("  File size %ld\n", file_size);
    } else {
        os.printf("WARNING - Could not get file size\n");
        file_size = 0;
    }

    this->played_cnt = 0;
    this->elapsed_secs = 0;
}

bool Player::progress_command( std::string& params, OutputStream& os )
{
    HELP("Display progress of sdcard print")

    // get options
    std::string options = shift_parameter( params );
    bool sdprinting = options.find_first_of("Bb") != std::string::npos;

    if(!playing_file && current_file_handler != NULL) {
        if(sdprinting)
            os.printf("SD printing byte %lu/%lu\n", played_cnt, file_size);
        else
            os.printf("SD print is paused at %lu/%lu\n", played_cnt, file_size);
        return true;

    } else if(!playing_file) {
        os.printf("Not currently playing\n");
        return true;
    }

    if(file_size > 0) {
        unsigned long est = 0;
        if(this->elapsed_secs > 10) {
            unsigned long bytespersec = played_cnt / this->elapsed_secs;
            if(bytespersec > 0)
                est = (file_size - played_cnt) / bytespersec;
        }

        unsigned int pcnt = (file_size - (file_size - played_cnt)) * 100 / file_size;
        // If -b or -B is passed, report in the format used by Marlin and the others.
        if (!sdprinting) {
            os.printf("file: %s, %u %% complete, elapsed time: %02lu:%02lu:%02lu", this->filename.c_str(), pcnt, this->elapsed_secs / 3600, (this->elapsed_secs % 3600) / 60, this->elapsed_secs % 60);
            if(est > 0) {
                os.printf(", est time: %02lu:%02lu:%02lu",  est / 3600, (est % 3600) / 60, est % 60);
            }
            os.printf("\n");
        } else {
            os.printf("SD printing byte %lu/%lu\n", played_cnt, file_size);
        }

    } else {
        os.printf("File size is unknown\n");
    }

    return true;
}

bool Player::abort_command( std::string& params, OutputStream& os )
{
    if(!playing_file && current_file_handler == NULL) {
        os.printf("Not currently playing\n");
        return true;
    }
    suspended = false;
    playing_file = false;
    played_cnt = 0;
    file_size = 0;
    this->filename = "";
    this->current_stream = NULL;
    fclose(current_file_handler);
    current_file_handler = NULL;
    if(params.empty()) {
        // clear out the block queue, will wait until queue is empty
        // MUST be called in on_main_loop to make sure there are no blocked main loops waiting to put something on the queue
        Conveyor::getInstance()->flush_queue();

        // now the position will think it is at the last received pos, so we need to do FK to get the actuator position and reset the current position
        Robot::getInstance()->reset_position_from_current_actuator_position();
        os.printf("Aborted playing or paused file. Please turn any heaters off manually\n");
    }
}

// called when in command thread context, we can issue commands here
// TODO this should be a thread as it is only called every 200ms
void Player::in_command_ctx()
{
    if(suspended && suspend_loops > 0) {
        // if we are suspended we need to allow main loop to cycle a few times then finish off the suspend processing
        if(--suspend_loops == 0) {
            suspend_part2();
            return;
        }
    }

    if( !this->booted ) {
        this->booted = true;
        if( this->on_boot_gcode_enable ) {
            OutputStream os; // null stream
            this->play_command(this->on_boot_gcode, os);
        } else {
            //THEKERNEL->serial->printf("On boot gcode disabled! skipping...\n");
        }
    }

    if( this->playing_file ) {
        if(Robot::getInstance()->is_halted()) {
            return;
        }

        char buf[130]; // lines upto 128 characters are allowed, anything longer is discarded
        bool discard = false;

        while(fgets(buf, sizeof(buf), this->current_file_handler) != NULL) {
            int len = strlen(buf);
            if(len == 0) continue; // empty line? should not be possible
            if(buf[len - 1] == '\n' || feof(this->current_file_handler)) {
                if(discard) { // we are discarding a long line
                    discard = false;
                    continue;
                }
                if(len == 1) continue; // empty line

                // TODO not sure where to output if requested as original stream may have gone
                // if(this->current_stream != nullptr) {
                //     this->current_os.printf("%s", buf);
                // }

                // TODO only if running from command thread context, if in own thread then need to feed to message queue
                dispatch_line(buf, os);
                played_cnt += len;
                return; // we feed one line per main loop

            } else {
                // discard long line
                if(this->current_stream != nullptr) { this->current_os.printf("Warning: Discarded long line\n"); }
                discard = true;
            }
        }

        this->playing_file = false;
        this->filename = "";
        played_cnt = 0;
        file_size = 0;
        fclose(this->current_file_handler);
        current_file_handler = NULL;
        this->current_stream = NULL;

        if(this->reply_stream != NULL) {
            // if we were printing from an M command from pronterface we need to send this back
            this->reply_os.printf("Done printing file\n");
            this->reply_stream = NULL;
        }
    }
}

bool Player::request(const char *key, void *value)
{
    if(strcmp("is_playing", key) == 0) {
        *(bool*)value = this->playing_file;
        return true;

    } else if(strcmp("is_suspended", key) == 0) {
        *(bool*)value = this->suspended;
        return true;

    } else if(strcmp("get_progress", key) == 0) {
        if(file_size > 0 && playing_file) {
            struct pad_progress p;
            p.elapsed_secs = this->elapsed_secs;
            p.percent_complete = (this->file_size - (this->file_size - this->played_cnt)) * 100 / this->file_size;
            p.filename = this->filename;
            *(struct pad_progress *)value = p;
            return true;
        }

    } else if(strcmp("abort_play", key) == 0) {
        OutputStream os;
        std::string cmd;
        abort_command(cmd, os);
        return true;
    }

    return false;
}

/**
Suspend a print in progress
1. send pause to upstream host, or pause if printing from sd
1a. loop on_main_loop several times to clear any buffered commmands TODO will need to change this as there is no main loop
2. wait for empty queue
3. save the current position, extruder position, temperatures - any state that would need to be restored
4. retract by specifed amount either on command line or in config
5. turn off heaters.
6. optionally run after_suspend gcode (either in config or on command line)

User may jog or remove and insert filament at this point, extruding or retracting as needed

*/
bool Player::suspend_command(std::string& params, OutputStream& os )
{
    HELP("suspend operation l parameter will leave heaters on")

    if(suspended) {
        os.printf("Already suspended\n");
        return;
    }

    os.printf("Suspending print, waiting for queue to empty...\n");

    // override the leave_heaters_on setting
    this->override_leave_heaters_on = (params == "l");

    suspended = true;
    if( this->playing_file ) {
        // pause an sd print
        this->playing_file = false;
        this->was_playing_file = true;
    } else {
        // send pause to upstream host
        os.printf("// action:pause\n");
        this->was_playing_file = false;
    }

    // we need to allow main loop to cycle a few times to clear any buffered commands in the serial streams etc
    suspend_loops = 10;
}

// this completes the suspend
void Player::suspend_part2()
{
    //  need to use streams here as the original stream may have changed
    printf("// Waiting for queue to empty (Host must stop sending)...\n");
    // wait for queue to empty
    Conveyor::getInstance()->wait_for_idle();

    printf("// Saving current state...\n");

    // save current XYZ position
    Robot::getInstance()->get_axis_position(this->saved_position);

    // save current extruder state
    PublicData::set_value( extruder_key, save_state_key, nullptr );

    // save state use M120
    Robot::getInstance()->push_state();

    // TODO retract by optional amount...

    this->saved_temperatures.clear();
    if(!this->leave_heaters_on && !this->override_leave_heaters_on) {
        // save current temperatures, get a vector of all the controllers data
        std::vector<struct pad_temperature> controllers;
        bool ok = PublicData::get_value(temperature_control_key, poll_controls_key, &controllers);
        if (ok) {
            // query each heater and save the target temperature if on
            for (auto &c : controllers) {
                // TODO see if in exclude list
                if(c.target_temperature > 0) {
                    this->saved_temperatures[c.id] = c.target_temperature;
                }
            }
        }

        // turn off heaters that were on
        for(auto& h : this->saved_temperatures) {
            float t = 0;
            PublicData::set_value( temperature_control_key, h.first, &t );
        }
    }

    // execute optional gcode if defined
    if(!after_suspend_gcode.empty()) {
        struct SerialMessage message;
        message.message = after_suspend_gcode;
        message.os = &(StreamOutput::NullStream);
        THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, &message );
    }

    printf("// Print Suspended, enter resume to continue printing\n");
}

/**
resume the suspended print
1. restore the temperatures and wait for them to get up to temp
2. optionally run before_resume gcode if specified
3. restore the position it was at and E and any other saved state
4. resume sd print or send resume upstream
*/
bool Player::resume_command(std::string& params, OutputStream& os )
{
    HELP("Resume a suspended operation")

    if(!suspended) {
        os.printf("Not suspended\n");
        return;
    }

    os.printf("resuming print...\n");

    // wait for them to reach temp
    if(!this->saved_temperatures.empty()) {
        // set heaters to saved temps
        for(auto& h : this->saved_temperatures) {
            float t = h.second;
            PublicData::set_value( temperature_control_key, h.first, &t );
        }
        os.printf("Waiting for heaters...\n");
        bool wait = true;
        uint32_t tus = us_ticker_read(); // mbed call
        while(wait) {
            wait = false;

            bool timeup = false;
            if((us_ticker_read() - tus) >= 1000000) { // print every 1 second
                timeup = true;
                tus = us_ticker_read(); // mbed call
            }

            for(auto& h : this->saved_temperatures) {
                struct pad_temperature temp;
                if(PublicData::get_value( temperature_control_key, current_temperature_key, h.first, &temp )) {
                    if(timeup)
                        os.printf("%s:%3.1f /%3.1f @%d ", temp.designator.c_str(), temp.current_temperature, ((temp.target_temperature == -1) ? 0.0 : temp.target_temperature), temp.pwm);
                    wait = wait || (temp.current_temperature < h.second);
                }
            }
            if(timeup) os.printf("\n");

            if(wait)
                THEKERNEL->call_event(ON_IDLE, this);

            if(THEKERNEL->is_halted()) {
                // abort temp wait and rest of resume
                THEKERNEL->streams->printf("Resume aborted by kill\n");
                Robot::getInstance()->pop_state();
                this->saved_temperatures.clear();
                suspended = false;
                return;
            }
        }
    }

    // execute optional gcode if defined
    if(!before_resume_gcode.empty()) {
        os.printf("Executing before resume gcode...\n");
        struct SerialMessage message;
        message.message = before_resume_gcode;
        message.os = &(StreamOutput::NullStream);
        THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, &message );
    }

    // Restore position
    os.printf("Restoring saved XYZ positions and state...\n");
    Robot::getInstance()->pop_state();
    bool abs_mode = Robot::getInstance()->absolute_mode; // what mode we were in
    // force absolute mode for restoring position, then set to the saved relative/absolute mode
    Robot::getInstance()->absolute_mode = true;
    {
        // NOTE position was saved in MCS so must use G53 to restore position
        char buf[128];
        snprintf(buf, sizeof(buf), "G53 G0 X%f Y%f Z%f", saved_position[0], saved_position[1], saved_position[2]);
        struct SerialMessage message;
        message.message = buf;
        message.os = &(StreamOutput::NullStream);
        THEKERNEL->call_event(ON_CONSOLE_LINE_RECEIVED, &message );
    }
    Robot::getInstance()->absolute_mode = abs_mode;

    // restore extruder state
    PublicData::set_value( extruder_key, restore_state_key, nullptr );

    os.printf("Resuming print\n");

    if(this->was_playing_file) {
        this->playing_file = true;
        this->was_playing_file = false;
    } else {
        // Send resume to host
        THEKERNEL->streams->printf("// action:resume\n");
    }

    // clean up
    this->saved_temperatures.clear();
    suspended = false;
}
