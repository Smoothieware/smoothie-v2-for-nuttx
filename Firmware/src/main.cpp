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

static int setup_CDC()
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
    if(OK != ret) return -1;

    sleep(3);

    /* Try to open the console */
    int fd;
    do {
        fd = open("/dev/ttyACM0", O_RDWR);
        /* ENOTCONN means that the USB device is not yet connected. Anything
         * else is bad.
         */

        if(fd < 0) {
            if(errno == ENOTCONN) {
                sleep(2);

            } else {
                printf("ttyACM0 Got error: %d\n", errno);
                return -1;
            }
        }
    } while (fd < 0);

    // we think we had a connection but due to a bug in Nuttx we may or may not
    // but it doesn't really seem to matter

    return fd;
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

// Only called by the dispatcher thread to receive incoming lines to process
bool receive_message_queue(mqd_t mqfd, const char **ppline, OutputStream **ppos)
{
    comms_msg_t msg_buffer;

    int nbytes = mq_receive(mqfd, (char *)&msg_buffer, sizeof(comms_msg_t), 0);
    if (nbytes < 0) {
        /* mq_receive failed.  If the error is because of EINTR then
         * it is not a failure.
         */

        if (errno != EINTR) {
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

    *ppline= msg_buffer.pline;
    *ppos= msg_buffer.pos;

    return true;
}


#include "GCode.h"
#include "GCodeProcessor.h"
#include "Dispatcher.h"
#include "CommandShell.h"

static GCodeProcessor gp;
static bool dispatch_line(OutputStream& os, const char *line)
{
    // see if a command
    if(islower(line[0]) || line[0] == '$') {
        if(!THEDISPATCHER.dispatch(line, os)) {
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

    // dispatch gcode to MotionControl and Planner
    for(auto& i : gcodes) {
        if(!THEDISPATCHER.dispatch(i, os)) {
            // no handler for this gcode, return ok - nohandler
            os.puts("ok - nohandler\n");
        }
    }

    return true;
}

static void usb_comms()
{
    printf("USB Comms thread running\n");

    int fd = setup_CDC();
    if(fd == -1) {
        printf("CDC setup failed\n");
        return;
    }

    // on first connect we send a welcome message
    static const char *welcome_message = "Welcome to Smoothie\nok\n";

    size_t n;
    char line[132];
    n = read(fd, line, 1);
    if(n == 1) {
        n = write(fd, welcome_message, strlen(welcome_message));
        if(n < 0) {
            printf("ttyACM0: Error writing welcome: %d\n", errno);
            close(fd);
            fd = -1;
            return;
        }

    } else {
        printf("ttyACM0: Error reading: %d\n", errno);
        fd = -1;
        return;
    }

    // get the message queue
    mqd_t mqfd= get_message_queue(false);

    // create an output stream that writes to the already open fd
    OutputStream os(fd);

    // now read lines and dispatch them
    size_t cnt = 0;
    bool discard= false;
    for(;;) {
        n = read(fd, &line[cnt], 1);
        if(n == 1) {
            if(discard) {
                // we discard long lines until we get the newline
                if(line[cnt] == '\n') discard= false;

            } else if(cnt >= sizeof(line) - 1) {
                // discard long lines
                discard= true;
                cnt= 0;
                os.puts("error:Discarding long line\n");

            } else if(line[cnt] == '\n') {
                line[cnt] = '\0'; // remove the \n and nul terminate
                // TODO line needs to be in a circular queue of lines as big or bigger than the mesage queue size
                // so it does not get re used before the command thread has dealt with it
                // We do not want to malloc/free all the time
                char *l= strdup(line);
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
    mqd_t mqfd= get_message_queue(false);

    // create an output stream that writes to cout/stdout
    OutputStream os(std::cout);

    // now read lines and dispatch them
    char line[132];
    size_t cnt = 0;
    size_t n;
    bool discard= false;
    for(;;) {
        n = read(0, &line[cnt], 1);

        if(n == 1) {
            if(discard) {
                // we discard long lines until we get the newline
                if(line[cnt] == '\n') discard= false;

            } else if(cnt >= sizeof(line) - 1) {
                // discard long lines
                discard= true;
                cnt= 0;
                os.puts("error:Discarding long line\n");

            } else if(line[cnt] == '\n') {
                line[cnt] = '\0'; // remove the \n and nul terminate
                // TODO line needs to be in a circular queue of lines as big or bigger than the message queue size
                // so it does not get re used before the command thread has dealt with it
                // We do not want to malloc/free all the time
                char *l= strdup(line);
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

static std::mutex m;
static std::condition_variable cv;
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
    mqd_t mqfd= get_message_queue(true);

    for(;;) {
        const char *line;
        OutputStream *os;

        if(receive_message_queue(mqfd, &line, &os)) {
            //printf("DEBUG: got line: %s\n", line);
            dispatch_line(*os, line);
            free((void *)line); // was strdup'd, FIXME we don't want to have do this

        }else{
            printf("receive_message_queue failed\n");
        }
    }

}

extern "C" int smoothie_main(int argc, char *argv[])
{
    // do C++ initialization for static constructors first
    // WARNING this is using miimum stack sioze of the main thread so minimize the use of statics and make sure they do not use a lot of stack
    // FIXME this is really NOT where this should be done
    up_cxxinitialize();

    printf("Smoothie V2.0alpha starting up\n");

    // this creates the timer devices and the sdcard device
    int ret = boardctl(BOARDIOC_INIT, 0);
    if(OK != ret) {
        printf("Error: intializing BOARDIOC_INIT\n");
    }

    // create the commandshell
    CommandShell shell;
    shell.initialize();

    // create the SlowTicker
    SlowTicker& slow_ticker= SlowTicker.getInstance();
    if(!slow_ticker.start()) {
        printf("Error: failed to start SlowTicker\n");
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

    sparam.sched_priority = 150; // (prio_min + prio_mid) / 2;
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

    printf("Exiting main\n");

    return 1;
}


