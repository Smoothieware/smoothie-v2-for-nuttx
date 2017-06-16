#pragma once

#include <map>
#include <functional>
#include <string>
#include <stdint.h>

#define THEDISPATCHER Dispatcher::getInstance()

class GCode;
class OutputStream;

class Dispatcher
{
public:
    // setup the Singleton instance
    static Dispatcher &getInstance()
    {
        static Dispatcher instance;
        return instance;
    }

    using Handler_t = std::function<bool(GCode&, OutputStream&)>;
    using Handlers_t = std::multimap<uint16_t, Handler_t>;
    enum HANDLER_NAME { GCODE_HANDLER, MCODE_HANDLER };
    Handlers_t::iterator add_handler(HANDLER_NAME gcode, uint16_t code, Handler_t fnc);

    using CommandHandler_t = std::function<bool(std::string&, OutputStream&)>;
    using CommandHandlers_t = std::multimap<std::string, CommandHandler_t>;
    CommandHandlers_t::iterator add_handler(std::string cmd, CommandHandler_t fnc);

    void remove_handler(HANDLER_NAME gcode, Handlers_t::iterator);
    bool dispatch(GCode &gc, OutputStream& os) const;
    bool dispatch(OutputStream& os, char cmd, uint16_t code, ...) const;
    bool dispatch(const char *line, OutputStream& os) const;
    bool load_configuration() const;
    void clear_handlers();

private:
    Dispatcher() {};
    Dispatcher(Dispatcher const &) = delete;
    void operator=(Dispatcher const &) = delete;
    bool handle_configuration_commands(GCode& gc, OutputStream& os) const;
    bool write_configuration(OutputStream& output_stream) const;
    bool load_configuration(OutputStream& output_stream) const;

    // use multimap as multiple handlers may be needed per gcode
    Handlers_t gcode_handlers;
    Handlers_t mcode_handlers;
    CommandHandlers_t command_handlers;
    mutable bool loaded_configuration{false};
};

