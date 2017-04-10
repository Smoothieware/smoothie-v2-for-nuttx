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

    using Handler_t = std::function<bool(GCode&)>;
    using Handlers_t = std::multimap<uint16_t, Handler_t>;
    enum HANDLER_NAME { GCODE_HANDLER, MCODE_HANDLER };

    Handlers_t::iterator addHandler(HANDLER_NAME gcode, uint16_t code, Handler_t fnc);
    void removeHandler(HANDLER_NAME gcode, Handlers_t::iterator);
    bool dispatch(GCode &gc) const;
    bool dispatch(char cmd, uint16_t code, ...) const;
    bool loadConfiguration() const;
    void clearHandlers();

private:
    Dispatcher() {};
    Dispatcher(Dispatcher const &) = delete;
    void operator=(Dispatcher const &) = delete;
    bool handleConfigurationCommands(GCode& gc) const;
    bool writeConfiguration(OutputStream& output_stream) const;
    bool loadConfiguration(OutputStream& output_stream) const;

    // use multimap as multiple handlers may be needed per gcode
    Handlers_t gcode_handlers;
    Handlers_t mcode_handlers;
    mutable bool loaded_configuration{false};
};

