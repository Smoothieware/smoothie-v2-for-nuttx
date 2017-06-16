#include "CommandShell.h"
#include "OutputStream.h"
#include "Dispatcher.h"

#include <functional>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


bool CommandShell::initialize()
{
    // register command handlers
    using std::placeholders::_1;
    using std::placeholders::_2;

    THEDISPATCHER.add_handler( "ls", std::bind( &CommandShell::ls_cmd, this, _1, _2) );
    THEDISPATCHER.add_handler( "rm", std::bind( &CommandShell::rm_cmd, this, _1, _2) );

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

bool CommandShell::ls_cmd(std::string& params, OutputStream& os)
{
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
            // struct stat buf;
            // if (stat(p->d_name, &buf) >= 0) {
            //     if (S_ISDIR(buf.st_mode)) {
            //         os.printf("/");
            //     }
            // } else if(opts.find("-s", 0, 2) != std::string::npos) {
            //     //os.printf(" %d", p->d_fsize);
            // }
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
    std::string fn = shift_parameter( params );
    int s = remove(fn.c_str());
    if (s != 0) os.printf("Could not delete %s\n", fn.c_str());
    return true;
}
