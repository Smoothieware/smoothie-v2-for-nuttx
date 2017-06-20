#pragma once

#include <map>
#include <string>
#include <vector>

class Module
{
public:
    Module(const char* group, const char* instance);
    Module(const char* group);
    virtual ~Module();

    // called to allow the module to read its configuration data
    virtual bool configure(){ return true; };

    // the system is entering or leaving halt/alarm mode flg == true if entering
    virtual void on_halt(bool flg) {};
    // request public data from module instance, tpy eof data requested is in key,
    // and the address of an appropriate returned data type is provided by the caller
    virtual bool request(const char *key, void *value) { return false; }

    // module registry function, to look up an instance of a module or a group of modules
    // returns nullptr if not found, otherwise returns a pointer to the module
    static Module* lookup(const char *group, const char *instance= nullptr);

    // lookup and return an array of modules that belong to group, returns empty if not found of if not a group
    static std::vector<Module*> lookup_group(const char *group);

    using instance_map_t = std::map<const std::string, Module*>; // map of module instances
    // either a map of module instances OR a pointer to a module, one will be nullptr the other won't be
    using modrec_t = struct { instance_map_t *map; Module *module; };

    // sends the on_halt event to all modules, flg is true if halt, false if cleared
    static void broadcast_halt(bool flg);

    bool was_added() const { return added; }

protected:
    // TODO do we realky want to store these here? currently needed for destructor
    std::string group_name, instance_name;

private:
    using registry_t = std::map<const std::string, modrec_t>;
    static registry_t registry;
    bool add(const char* group, const char* instance= nullptr);

    // specifies whether this is registered as a single module or a group of modules
    bool single;
    bool added;
};
