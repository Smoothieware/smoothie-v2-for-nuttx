#include "main.h"

#include <nuttx/config.h>
#include <sys/boardctl.h>
#include <nuttx/usb/cdcacm.h>
#include <nuttx/init.h>
#include <nuttx/arch.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <iostream>

#include "Module.h"

// static std::mutex m;
// static std::condition_variable cv;

//set in uart thread to signal command_thread to print a query response
static bool do_query = false;
static OutputStream *query_os = nullptr;

static bool setup_CDC(int& rfd, int& wfd)
{
    // create the CDC device
    struct boardioc_usbdev_ctrl_s ctrl;
    void *handle;
    int ret;

    /* Initialize the USB serial driver */

    ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
    ctrl.action   = BOARDIOC_USBDEV_CONNECT;
    ctrl.instance = 0; // CONFIG_NSH_USBDEV_MINOR;
    ctrl.handle   = &handle;

    ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
    if(OK != ret) return false;

    sleep(3);

    /* Try to open the console */
    int fd;
    do {
        fd = open("/dev/ttyACM0", O_RDONLY);
        /* ENOTCONN means that the USB device is not yet connected. Anything
         * else is bad.
         */

        if(fd < 0) {
            if(errno == ENOTCONN) {
                sleep(2);

            } else {
                printf("ttyACM0 Got error opening for read: %d\n", errno);
                return false;
            }
        }
    } while (fd < 0);

    // we think we had a connection but due to a bug in Nuttx we may or may not
    // but it doesn't really seem to matter

    rfd= fd;

    // now open for write
    fd = open("/dev/ttyACM0", O_WRONLY);
    if(fd < 0) {
        printf("ttyACM0 got error opening for write: %d\n", errno);
        return false;
    }

    wfd= fd;
    return true;
}

#include "OutputStream.h"
using comms_msg_t = struct {const char* pline; OutputStream *pos; };

#include <pthread.h>
#include <mqueue.h>
mqd_t get_message_queue(bool read)
{
    struct mq_attr attr;

    /* Fill in attributes for message queue */

    attr.mq_maxmsg  = 4;
    attr.mq_msgsize = sizeof(comms_msg_t);
    attr.mq_flags   = 0;

    /* Set the flags for the open of the queue.
     * Make it a blocking open on the queue, meaning it will block if
     * this process tries to send to the queue and the queue is full.
     *
     *   O_CREAT - the queue will get created if it does not already exist.
     *   O_WRONLY - we are only planning to write to the queue.
     *
     * Open the queue, and create it if it hasn't already been created.
     */
    int flgs = read ? O_RDONLY : O_WRONLY;
    mqd_t mqfd = mq_open("comms_q", flgs | O_CREAT, 0666, &attr);
    if (mqfd == (mqd_t) - 1) {
        printf("get_message_queue: ERROR mq_open failed\n");
    }

    return mqfd;
}

// can be called by several threads to submit messages to the dispatcher
// This call will block until there is room in the queue
// eg USB serial, UART serial, Network, SDCard player thread
bool send_message_queue(mqd_t mqfd, const char *pline, OutputStream *pos)
{
    comms_msg_t msg_buffer{pline, pos};

    int status = mq_send(mqfd, (const char *)&msg_buffer, sizeof(comms_msg_t), 42);
    if (status < 0) {
        printf("send_message_queue: ERROR mq_send failure=%d\n", status);
        return false;
    }

    return true;
}

// Only called by the command thread to receive incoming lines to process
static bool receive_message_queue(mqd_t mqfd, const char **ppline, OutputStream **ppos)
{
    comms_msg_t msg_buffer;
    struct timespec ts;
    int status = clock_gettime(CLOCK_REALTIME, &ts);
    if (status != 0) {
        printf("receive_message_queue: ERROR clock_gettime failed\n");
    }
    //                1000000000  // 1 second in ns
    ts.tv_nsec     +=  200000000; // 200ms timeout
    if(ts.tv_nsec  >= 1000000000) {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec  += 1;
    }

    //int nbytes = mq_receive(mqfd, (char *)&msg_buffer, sizeof(comms_msg_t), 0);
    int nbytes = mq_timedreceive(mqfd, (char *)&msg_buffer, sizeof(comms_msg_t), 0, &ts);
    if (nbytes < 0) {
        // mq_receive failed.  If the error is because of EINTR or ETIMEDOUT then it is not a failure. but we return anyway
        if (errno == ETIMEDOUT) {
            // timeout is ok
            return false;

        } else if (errno != EINTR) {
            printf("receive_message_queue: ERROR mq_receive failure, errno=%d\n", errno);
            return false;

        } else {
            printf("receive_message_queue: mq_receive interrupted!\n");
            return false;
        }

    } else if (nbytes != sizeof(comms_msg_t)) {
        printf("receive_message_queue: mq_receive return bad size %d\n", nbytes);
        return false;
    }

    *ppline = msg_buffer.pline;
    *ppos = msg_buffer.pos;

    return true;
}


#include "GCode.h"
#include "GCodeProcessor.h"
#include "Dispatcher.h"
#include "Robot.h"

// TODO maybe move to Dispatcher
static GCodeProcessor gp;
// can be called by modules when in command thread context
bool dispatch_line(OutputStream& os, const char *line)
{
    // TODO map some special M codes to commands as they violate the gcode spec and pass a string parameter
    // M23, M32, M117 => m23, m32, m117 and handle as a command

    // see if a command
    if(islower(line[0]) || line[0] == '$') {

        // we could handle this in CommandShell
        if(line[0] == '$' && strlen(line) >= 2) {
            if(line[1] == 'X') {
                // handle $X
                if(Module::is_halted()) {
                    Module::broadcast_halt(false);
                    os.puts("[Caution: Unlocked]\nok\n");
                } else {
                    os.puts("ok\n");
                }
                return true;
            }
        }

        // dispatch command
        if(!THEDISPATCHER->dispatch(line, os)) {
            if(line[0] == '$') {
                os.puts("error:Invalid statement\n");
            } else {
                os.printf("error:Unsupported command - %s\n", line);
            }
        }

        return true;
    }

    // Handle Gcode
    GCodeProcessor::GCodes_t gcodes;

    // Parse gcode
    if(!gp.parse(line, gcodes)) {
        // line failed checksum, send resend request
        os.printf("rs N%d\n", gp.get_line_number() + 1);
        return true;

    } else if(gcodes.empty()) {
        // if gcodes is empty then was a M110, just send ok
        os.puts("ok\n");
        return true;
    }

    // dispatch gcodes
    // NOTE return one ok per line instead of per GCode only works for regular gcodes like G0-G3, G92 etc
    // gcodes returning data like M114 should NOT be put on multi gcode lines.
    int ngcodes= gcodes.size();
    for(auto& i : gcodes) {
        //i.dump(os);
        if(i.has_m() || i.has_g()) {
            // if this is a multi gcode line then dispatch must not send ok unless this is the last one
            if(!THEDISPATCHER->dispatch(i, os, ngcodes == 1)) {
                // no handler processed this gcode, return ok - ignored
                if(ngcodes == 1) os.puts("ok - ignored\n");
            }

        } else {
            // if it has neither g or m then it was a blank line or comment
            os.puts("ok\n");
        }
        --ngcodes;
    }

    return true;
}

#include <vector>
std::vector<OutputStream*> output_streams;

static void usb_comms()
{
    printf("USB Comms thread running\n");
    int rfd, wfd;

    if(!setup_CDC(rfd, wfd)) {
        printf("CDC setup failed\n");
        return;
    }

    // on first connect we send a welcome message
    static const char *welcome_message = "Welcome to Smoothie\nok\n";

    size_t n;
    char line[132];
    n = read(rfd, line, 1);
    if(n == 1) {
        n = write(wfd, welcome_message, strlen(welcome_message));
        if(n < 0) {
            printf("ttyACM0: Error writing welcome: %d\n", errno);
            close(rfd);
            close(wfd);
            return;
        }

    } else {
        printf("ttyACM0: Error reading: %d\n", errno);
        close(rfd);
        close(wfd);
        return;
    }

    // get the message queue
    mqd_t mqfd = get_message_queue(false);

    // create an output stream that writes to the already open fd
    OutputStream os(wfd);
    output_streams.push_back(&os);

    // now read lines and dispatch them
    size_t cnt = 0;
    bool discard = false;
    for(;;) {
        n = read(rfd, &line[cnt], 1);
        if(n == 1) {
            if(line[cnt] == 24) { // ^X
                if(!Module::is_halted()) {
                    Module::broadcast_halt(true);
                    os.puts("ALARM: Abort during cycle\n");
                }
                discard = false;
                cnt = 0;

            } else if(line[cnt] == '?') {
                query_os = &os; // we need to let it know where to send response back to TODO maybe a race condition if both USB and uart send ?
                do_query = true;

            } else if(discard) {
                // we discard long lines until we get the newline
                if(line[cnt] == '\n') discard = false;

            } else if(cnt >= sizeof(line) - 1) {
                // discard long lines
                discard = true;
                cnt = 0;
                os.puts("error:Discarding long line\n");

            } else if(line[cnt] == '\n') {
                line[cnt] = '\0'; // remove the \n and nul terminate
                // TODO line needs to be in a circular queue of lines as big or bigger than the mesage queue size
                // so it does not get re used before the command thread has dealt with it
                // We do not want to malloc/free all the time
                char *l = strdup(line);
                send_message_queue(mqfd, l, &os);
                cnt = 0;

            } else if(line[cnt] == '\r') {
                // ignore CR
                continue;

            } else if(line[cnt] == 8 || line[cnt] == 127) { // BS or DEL
                if(cnt > 0) --cnt;

            } else {
                ++cnt;
            }
        } else {
            printf("ttyACM0: read error: %d\n", errno);
            break;
        }
    }

    printf("Comms thread exiting\n");
}

static void uart_comms()
{
    printf("UART Comms thread running\n");

    // get the message queue
    mqd_t mqfd = get_message_queue(false);

    // create an output stream that writes to stdout which is the uart
    OutputStream os(1); // use stdout fd, not this which is buffered (&std::cout);
    output_streams.push_back(&os);

    // now read lines and dispatch them
    char line[132];
    size_t cnt = 0;
    size_t n;
    bool discard = false;
    for(;;) {
        n = read(0, &line[cnt], 1);

        if(n == 1) {
            if(line[cnt] == 24) { // ^X
                if(!Module::is_halted()) {
                    Module::broadcast_halt(true);
                    os.puts("ALARM: Abort during cycle\n");
                }
                discard = false;
                cnt = 0;

            } else if(line[cnt] == '?') {
                query_os = &os; // we need to let it know where to send response back to TODO maybe a race condition if both USB and uart send ?
                do_query = true;

            // } else if(line[cnt] == '!') {
            //     do_feed_hold(true);

            // } else if(line[cnt] == '~') {
            //     do_feed_hold(false);

            } else if(discard) {
                // we discard long lines until we get the newline
                if(line[cnt] == '\n') discard = false;

            } else if(cnt >= sizeof(line) - 1) {
                // discard long lines
                discard = true;
                cnt = 0;
                os.puts("error:Discarding long line\n");

            } else if(line[cnt] == '\n') {
                line[cnt] = '\0'; // remove the \n and nul terminate
                // TODO line needs to be in a circular queue of lines as big or bigger than the message queue size
                // so it does not get re used before the command thread has dealt with it
                // We do not want to malloc/free all the time
                char *l = strdup(line);
                send_message_queue(mqfd, l, &os);
                cnt = 0;

            } else if(line[cnt] == '\r') {
                // ignore CR
                continue;

            } else if(line[cnt] == 8 || line[cnt] == 127) { // BS or DEL
                if(cnt > 0) --cnt;

            } else {
                ++cnt;
            }

        } else {
            printf("UART: read error: %d\n", errno);
            break;
        }
    }

    printf("UART Comms thread exiting\n");
}


// this prints the string to all consoles that are connected and active
// must be called in command thread context
void print_to_all_consoles(const char *str)
{
    for(auto i : output_streams) {
        i->puts(str);
    }
}

#include "Conveyor.h"
#include "Pin.h"

// Define the activity/idle indicator led
static Pin *idle_led= nullptr;

/*
 * All commands must be executed in the context of this thread. It is equivalent to the main_loop in v1.
 * Commands are sent to this thread via the message queue from things that can block (like I/O)
 * How to queue things from interupts like from Switch?
 * 1. We could have a timeout on the I/O queue of about 100-200ms and check an internal queue for commands
 * 2. we could call a on_main_loop to all registed modules.
 * Not fond of 2 and 1 requires somw form of locking so interrupts can access the queue too.
 */
static void *commandthrd(void *)
{
    printf("Command thread running\n");
    // {
    //     // Manual unlocking is done before notifying, to avoid waking up
    //     // the waiting thread only to block again (see notify_one for details)
    //     std::unique_lock<std::mutex> lk(m);
    //     lk.unlock();
    //     cv.notify_one();
    // }

    // get the message queue
    mqd_t mqfd = get_message_queue(true);

    for(;;) {
        const char *line;
        OutputStream *os;

        // This will timeout after 200 ms
        if(receive_message_queue(mqfd, &line, &os)) {
            //printf("DEBUG: got line: %s\n", line);
            dispatch_line(*os, line);
            free((void *)line); // was strdup'd, FIXME we don't want to have do this
        } else {
            // timed out or other error

            if(idle_led != nullptr) {
                // toggle led to show we are alive, but idle
                idle_led->set(!idle_led->get());
            }
        }

        // set in comms thread, and executed here to avoid thread clashes
        // the trouble with this is that ? does not reply if a long command is blocking above call to dispatch_line
        // test comamnds for instance or a long line when the queue is full or G4 etc
        // so long as safe_sleep() is called then this will still be handled
        if(do_query) {
            std::string r;
            Robot::getInstance()->get_query_string(r);
            query_os->puts(r.c_str());
            do_query = false;
        }

        // call in_command_ctx for all modules that want it
        Module::broadcast_in_commmand_ctx();

        // we check the queue to see if it is ready to run
        // we specifically deal with this in append_block, but need to check for other places
        // This used to be done in on_idle which never blocked
        Conveyor::getInstance()->check_queue();
    }
}

// called only in command thread context, it will sleep (and yield) thread but will also
// process things like query
void safe_sleep(uint32_t ms)
{
    // here we need to sleep (and yield) for 10ms then check if we need to handle the query command
    while(ms > 0) {
        usleep(10000); // 10 ms sleep (minimum anyway due to thread slice time)
        if(do_query) {
            std::string r;
            Robot::getInstance()->get_query_string(r);
            query_os->puts(r.c_str());
            do_query = false;
        }

        if(ms > 10) {
            ms -= 10;
        }else{
            break;
        }
    }
}

#include "CommandShell.h"
#include "SlowTicker.h"
#include "StepTicker.h"
#include "ConfigReader.h"
#include "Switch.h"
#include "Planner.h"
#include "Robot.h"
#include "KillButton.h"
#include "Extruder.h"
#include "TemperatureControl.h"
#include "Adc.h"
#include "Pwm.h"
#include "CurrentControl.h"
#include "Laser.h"
#include "Endstops.h"

#include "main.h"
#include <sys/mount.h>
#include <fstream>

void configureSPIFI();
float get_pll1_clk();

#include STRING_CONFIG_H
static std::string str(string_config);
static std::stringstream ss(str);


static int smoothie_startup(int, char **)
{
    // do C++ initialization for static constructors first
    // FIXME this is really NOT where this should be done
    up_cxxinitialize();

    printf("Smoothie V2.0alpha Build for %s - starting up\n", BUILD_TARGET);
    get_pll1_clk();

    // create the SlowTicker here as it is used by some modules
    SlowTicker *slow_ticker = new SlowTicker();

    // create the StepTicker, don't start it yet
    StepTicker *step_ticker = new StepTicker();
#ifdef DEBUG
    // when debug is enabled we cannot run stepticker at full speed
    step_ticker->set_frequency(10000); // 10KHz
#else
    step_ticker->set_frequency(100000); // 100KHz
#endif
    step_ticker->set_unstep_time(1); // 1us step pulse by default

    // configure the Dispatcher
    new Dispatcher();

    bool ok = false;

    // open the config file
    do {
#if 0
        int ret = mount("/dev/mmcsd0", "/sd", "vfat", 0, nullptr);
        if(0 != ret) {
            std::cout << "Error mounting: " << "/dev/mmcsd0: " << ret << "\n";
            break;
        }

        std::fstream fs;
        fs.open("/sd/test-config.ini", std::fstream::in);
        if(!fs.is_open()) {
            std::cout << "Error opening file: " << "/sd/config.ini" << "\n";
            // unmount sdcard
            umount("/sd");
            break;
        }


        ConfigReader cr(fs);
        printf("Starting configuration of modules from sdcard...\n");
#else
        ConfigReader cr(ss);
        printf("Starting configuration of modules from memory...\n");
#endif

        {
            // get general system settings
            ConfigReader::section_map_t m;
            if(cr.get_section("general", m)) {
                bool f= cr.get_bool(m, "grbl_mode", false);
                THEDISPATCHER->set_grbl_mode(f);
                printf("grbl mode %s\n", f?"set":"not set");
            }
        }

        printf("configure the planner\n");
        Planner *planner = new Planner();
        planner->configure(cr);

        printf("configure the conveyor\n");
        Conveyor *conveyor = new Conveyor();
        conveyor->configure(cr);

        printf("configure the robot\n");
        Robot *robot = new Robot();
        if(!robot->configure(cr)) {
            printf("ERROR: Configuring robot failed\n");
            break;
        }

        ///////////////////////////////////////////////////////////
        // configure other modules here

        {
            printf("configure switches\n");
            // this creates any configured switches then we can remove it
            Switch switches("switch loader");
            if(!switches.configure(cr)) {
                printf("INFO: no switches loaded\n");
            }
        }

        {
            printf("configure extruder\n");
            // this creates any configured extruders then we can remove it
            Extruder ex("extruder loader");
            if(!ex.configure(cr)) {
                printf("INFO: no Extruders loaded\n");
            }
        }

        {
            printf("configure temperature control\n");
            if(Adc::setup()) {
                // this creates any configured temperature controls then we can remove it
                TemperatureControl tc("temperature control loader");
                if(!tc.configure(cr)) {
                    printf("INFO: no Temperature Controls loaded\n");
                }
            }else{
                printf("ERROR: ADC failed to setup\n");
            }
        }

        printf("configure endstops\n");
        Endstops *endstops = new Endstops();
        if(!endstops->configure(cr)) {
            printf("INFO: No endstops enabled\n");
            delete endstops;
            endstops = nullptr;
        }

        printf("configure kill button\n");
        KillButton *kill_button = new KillButton();
        if(!kill_button->configure(cr)) {
            printf("INFO: No kill button enabled\n");
            delete kill_button;
            kill_button = nullptr;
        }

        {
            // Pwm needs to be initialized, there can only be one frequency
            float freq= 10000; // default is 10KHz
            ConfigReader::section_map_t m;
            if(cr.get_section("pwm", m)) {
                freq= cr.get_float(m, "frequency", 10000);
            }
            Pwm::setup(freq);
            printf("INFO: PWM frequency set to %f Hz\n", freq);
        }

        printf("configure current control\n");
        CurrentControl *current_control = new CurrentControl();
        if(!current_control->configure(cr)) {
            printf("INFO: No current controls configured\n");
            delete current_control;
            current_control = nullptr;
        }

        printf("configure laser\n");
        Laser *laser = new Laser();
        if(!laser->configure(cr)) {
            printf("INFO: No laser configured\n");
            delete laser;
            laser = nullptr;
        }

        {
            // configure system leds (if any)
            ConfigReader::section_map_t m;
            if(cr.get_section("system leds", m)) {
                std::string p= cr.get_string(m, "idle_led", "nc");
                idle_led= new Pin(p.c_str(), Pin::AS_OUTPUT);
                if(!idle_led->connected()) {
                    delete idle_led;
                    idle_led= nullptr;
                }
            }
        }


        // end of module creation and configuration
        ////////////////////////////////////////////////////////////////

        // close the file stream
        //fs.close();

        // unmount sdcard
        //umount("/sd");

        // initialize planner before conveyor this is when block queue is created
        // which needs to know how many actuators there are, which it gets from robot
        if(!planner->initialize(robot->get_number_registered_motors())) {
            printf("ERROR: planner failed to initialize, out of memory?\n");
            break;
        }

        // start conveyor last
        conveyor->start();

        printf("...Ending configuration of modules\n");
        ok = true;
    } while(0);

    // create the commandshell, it is dependent on some of the above
    CommandShell *shell = new CommandShell();
    shell->initialize();

    if(ok) {
        // start the timers
        if(!slow_ticker->start()) {
            printf("Error: failed to start SlowTicker\n");
        }

        if(!step_ticker->start()) {
            printf("Error: failed to start StepTicker\n");
        }

        if(!Adc::start()) {
            printf("Error: failed to start ADC\n");
        }
    }

    // launch the command thread that executes all incoming commands
    // We have to do this the long way as we want to set the stack size and priority
    pthread_t command_thread;
    void *result;
    pthread_attr_t attr;
    struct sched_param sparam;
    int status;

    // int prio_min = sched_get_priority_min(SCHED_RR);
    // int prio_max = sched_get_priority_max(SCHED_RR);
    // int prio_mid = (prio_min + prio_max) / 2;

    status = pthread_attr_init(&attr);
    if (status != 0) {
        printf("main: pthread_attr_init failed, status=%d\n", status);
    }

    status = pthread_attr_setstacksize(&attr, 10000);
    if (status != 0) {
        printf("main: pthread_attr_setstacksize failed, status=%d\n", status);
    }

    status = pthread_attr_setschedpolicy(&attr, SCHED_RR);
    if (status != OK) {
        printf("main: pthread_attr_setschedpolicy failed, status=%d\n", status);
    } else {
        printf("main: Set command thread policy to SCHED_RR\n");
    }

    sparam.sched_priority = 90; // set lower than comms threads... 150; // (prio_min + prio_mid) / 2;
    status = pthread_attr_setschedparam(&attr, &sparam);
    if (status != OK) {
        printf("main: pthread_attr_setschedparam failed, status=%d\n", status);
    } else {
        printf("main: Set command thread priority to %d\n", sparam.sched_priority);
    }

    status = pthread_create(&command_thread, &attr, commandthrd, NULL);
    if (status != 0) {
        printf("main: pthread_create failed, status=%d\n", status);
    }

    // wait for command thread to start
    // std::unique_lock<std::mutex> lk(m);
    // cv.wait(lk);
    // printf("Command thread started\n");

    // Start comms threads
    // fixed stack size of 4k each
    std::thread usb_comms_thread(usb_comms);
    std::thread uart_comms_thread(uart_comms);

    sched_param sch_params;
    // sch_params.sched_priority = 10;
    // if(pthread_setschedparam(usb_comms_thread.native_handle(), SCHED_RR, &sch_params)) {
    //     printf("Failed to set Thread scheduling : %s\n", std::strerror(errno));
    // }

    int policy;
    status = pthread_getschedparam(usb_comms_thread.native_handle(), &policy, &sch_params);
    printf("pthread get params: status= %d, policy= %d, priority= %d\n", status, policy, sch_params.sched_priority);

    // Join the comms thread with the main thread
    usb_comms_thread.join();
    uart_comms_thread.join();
    pthread_join(command_thread, &result);

    printf("Exiting startup thread\n");

    return 1;
}

extern "C" int smoothie_main(int argc, char *argv[])
{
    configureSPIFI(); // setup the winbond SPIFI to max speed

    int ret = boardctl(BOARDIOC_INIT, 0);
    if(OK != ret) {
        printf("ERROR: BOARDIOC_INIT falied\n");
    }

    // We need to do this as the cxxinitialize takes more stack than the default task has,
    // this causes corruption and random crashes
    task_create("smoothie_task", SCHED_PRIORITY_DEFAULT,
                20000, // stack size may need to increase
                (main_t)smoothie_startup,
                (FAR char * const *)NULL);

    return 1;
}
