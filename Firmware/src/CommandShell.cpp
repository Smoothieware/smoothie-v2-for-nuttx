#include "CommandShell.h"
#include "OutputStream.h"
#include "Dispatcher.h"
#include "Module.h"

#include <functional>
#include <set>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define HELP(m) if(params == "-h") { os.printf("%s\n", m); return true; }

CommandShell::CommandShell()
{
    mounted = false;
}

bool CommandShell::initialize()
{
    // register command handlers
    using std::placeholders::_1;
    using std::placeholders::_2;

    THEDISPATCHER.add_handler( "help", std::bind( &CommandShell::help_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "ls", std::bind( &CommandShell::ls_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "rm", std::bind( &CommandShell::rm_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "mem", std::bind( &CommandShell::mem_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "mount", std::bind( &CommandShell::mount_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "cat", std::bind( &CommandShell::cat_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "md5sum", std::bind( &CommandShell::md5sum_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "switch", std::bind( &CommandShell::switch_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "gpio", std::bind( &CommandShell::gpio_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "modules", std::bind( &CommandShell::modules_cmd, this, _1, _2) );

    return true;
}

// Get the first parameter, and remove it from the original string
// TODO move to utils
std::string shift_parameter( std::string &parameters )
{
    size_t beginning = parameters.find_first_of(" ");
    if( beginning == std::string::npos ) {
        std::string temp = parameters;
        parameters = "";
        return temp;
    }
    std::string temp = parameters.substr( 0, beginning );
    parameters = parameters.substr(beginning + 1, parameters.size());
    return temp;
}

// lists all the registered commands
bool CommandShell::help_cmd(std::string& params, OutputStream& os)
{
    HELP("Show available commands");
    auto cmds= THEDISPATCHER.get_commands();
    for(auto& i : cmds) {
        os.printf("%s\n", i.c_str());
        // Display the help string for each command
        //    std::string cmd(i);
        //    cmd.append(" -h");
        //    THEDISPATCHER.dispatch(cmd.c_str(), os);
    }
    os.puts("\nuse cmd -h to get help on that command\n");

    return true;
}

bool CommandShell::ls_cmd(std::string& params, OutputStream& os)
{
    HELP("list files: -s show size");
    std::string path, opts;
    while(!params.empty()) {
        std::string s = shift_parameter( params );
        if(s.front() == '-') {
            opts.append(s);
        } else {
            path = s;
            if(!params.empty()) {
                path.append(" ");
                path.append(params);
            }
            break;
        }
    }

    DIR *d;
    struct dirent *p;
    d = opendir(path.c_str());
    if (d != NULL) {
        while ((p = readdir(d)) != NULL) {
            os.printf("%s", p->d_name);
            struct stat buf;
            std::string sp = path + "/" + p->d_name;
            if (stat(sp.c_str(), &buf) >= 0) {
                if (S_ISDIR(buf.st_mode)) {
                    os.printf("/");

                } else if(opts.find("-s", 0, 2) != std::string::npos) {
                    os.printf(" %d", buf.st_size);
                }
            } else {
                os.printf(" - Could not stat: %s", sp.c_str());
            }
            os.printf("\n");
        }
        closedir(d);
    } else {
        os.printf("Could not open directory %s\n", path.c_str());
    }

    return true;
}

bool CommandShell::rm_cmd(std::string& params, OutputStream& os)
{
    HELP("delete file");
    std::string fn = shift_parameter( params );
    int s = remove(fn.c_str());
    if (s != 0) os.printf("Could not delete %s\n", fn.c_str());
    return true;
}

bool CommandShell::mem_cmd(std::string& params, OutputStream& os)
{
    HELP("show memory allocation");
    struct mallinfo mem = mallinfo();
    os.printf("             total       used       free    largest\n");
    os.printf("Mem:   %11d%11d%11d%11d\n", mem.arena, mem.uordblks, mem.fordblks, mem.mxordblk);
    return true;
}

#include <sys/mount.h>
#include <sys/boardctl.h>
bool CommandShell::mount_cmd(std::string& params, OutputStream& os)
{
    HELP("mount sdcard on /sd (or unmount if already mounted)");

    const char g_target[]         = "/sd";
    const char g_filesystemtype[] = "vfat";
    const char g_source[]         = "/dev/mmcsd0";

    if(mounted) {
        os.printf("Already mounted, unmounting\n");
        umount(g_target);
        mounted= false;
        return true;
    }

    int ret;
    // ret = boardctl(BOARDIOC_INIT, 0);
    // if(OK != ret) {
    //     os.printf("Failed to INIT SDIO\n");
    //     return true;
    // }

    ret = mount(g_source, g_target, g_filesystemtype, 0, nullptr);
    if(0 == ret) {
        mounted = true;
        os.printf("Mounted %s on %s\n", g_source, g_target);

    }else{
        os.printf("Failed to mount sdcard\n");
    }
    return true;
}

bool CommandShell::cat_cmd(std::string& params, OutputStream& os)
{
    HELP("display file: nnn option will show first nnn lines");
    // Get parameters ( filename and line limit )
    std::string filename          = shift_parameter( params );
    std::string limit_parameter   = shift_parameter( params );
    int limit = -1;

    if ( limit_parameter != "" ) {
        char *e = NULL;
        limit = strtol(limit_parameter.c_str(), &e, 10);
        if (e <= limit_parameter.c_str())
            limit = -1;
    }

    // Open file
    FILE *lp = fopen(filename.c_str(), "r");
    if (lp != NULL) {
        char buffer[132];
        int newlines = 0;
        // Print each line of the file
        while (fgets (buffer, sizeof(buffer)-1, lp) != nullptr) {
            os.puts(buffer);
            if ( limit > 0 && ++newlines >= limit ) {
                break;
            }
        };
        fclose(lp);

    }else{
        os.printf("File not found: %s\n", filename.c_str());
    }

    return true;
}

#include "md5.h"
bool CommandShell::md5sum_cmd(std::string& params, OutputStream& os)
{
    HELP("calculate the md5sum of given filename");

    // Open file
    FILE *lp = fopen(params.c_str(), "r");
    if (lp != NULL) {
        MD5 md5;
        uint8_t buf[64];
        do {
            size_t n= fread(buf, 1, sizeof buf, lp);
            if(n > 0) md5.update(buf, n);
        } while(!feof(lp));

        os.printf("%s %s\n", md5.finalize().hexdigest().c_str(), params.c_str());
        fclose(lp);

    }else{
        os.printf("File not found: %s\n", params.c_str());
    }

    return true;
}

#include "Switch.h"
// set or get switch state for a named switch
bool CommandShell::switch_cmd(std::string& params, OutputStream& os)
{
    HELP("list switches or get/set named switch. if 2nd parameter is on/off it sets state if it is numeric it sets value");

    std::string name = shift_parameter( params );
    std::string value = shift_parameter( params );

    if(name.empty()) {
        // just list all the switches
        std::vector<Module*> mv = Module::lookup_group("switch");
        if(mv.size() > 0) {
            for(auto m : mv) {
                Switch *s= static_cast<Switch*>(m);
                os.printf("%s:\n", m->get_instance_name());
                os.printf(" %s\n", s->get_info().c_str());
            }
        }else{
            os.printf("No switches found\n");
        }

        return true;
    }

    Module *m= Module::lookup("switch", name.c_str());
    if(m == nullptr) {
        os.printf("no such switch: %s\n", name.c_str());
        return true;
    }

    bool ok = false;
    if(value.empty()) {
        // get switch state
        bool state;
        ok= m->request("state", &state);
        if (!ok) {
            os.printf("unknown command %s.\n", "state");
            return true;
        }
        os.printf("switch %s is %d\n", name.c_str(), state);

    }else{
        const char *cmd;
        // set switch state
        if(value == "on" || value == "off") {
            bool b = value == "on";
            cmd= "set-state";
            ok =  m->request(cmd, &b);

        } else {
            float v = strtof(value.c_str(), NULL);
            cmd= "set-value";
            ok = m->request(cmd, &v);
        }

        if (ok) {
            os.printf("switch %s set to: %s\n", name.c_str(), value.c_str());
        } else {
            os.printf("unknown command %s.\n", cmd);
        }
    }

    return true;
}

// set or get gpio
bool CommandShell::gpio_cmd(std::string& params, OutputStream& os)
{
    HELP("set and get gpio pins: use GPIO5[14] | gpio5_14 | P4_10 | p4.10 out/in [on/off]");

    std::string gpio = shift_parameter( params );
    std::string dir = shift_parameter( params );

    if(gpio.empty()) return false;

    if(dir.empty() || dir == "in") {
        // read pin
        Pin pin(gpio.c_str());
        if(!pin.connected()) {
            os.printf("Not a valid GPIO\n");
            return true;
        }
        pin.as_input();
        os.printf("%s: %d\n", pin.to_string().c_str(), pin.get());
        return true;
    }

    if(dir == "out") {
        std::string v = shift_parameter( params );
        if(v.empty()) return false;
        Pin pin(gpio.c_str());
        if(!pin.connected()) {
            os.printf("Not a valid GPIO\n");
            return true;
        }
        pin.as_output();
        bool b= (v == "on");
        pin.set(b);
        os.printf("%s: set to %d\n", pin.to_string().c_str(), pin.get());
        return true;
    }

    return false;
}

bool CommandShell::modules_cmd(std::string& params, OutputStream& os)
{
    HELP("List all registered modules\n");

    std::vector<std::string> l= Module::print_modules();

    if(l.empty()) {
        os.printf("No modules found\n");
        return true;
    }

    for(auto& i : l) {
        os.printf("%s\n", i.c_str());
    }

    return true;
}
