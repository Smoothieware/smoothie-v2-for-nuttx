#include "../easyunit/test.h"

#include <nuttx/config.h>
#include <sys/boardctl.h>
#include <nuttx/usb/cdcacm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

TEST(USBCDCTest, basic)
{
    #if 0
    struct boardioc_usbdev_ctrl_s ctrl;
    FAR void *handle;
    int ret;

    /* Initialize the USB serial driver */

    ctrl.usbdev   = BOARDIOC_USBDEV_CDCACM;
    ctrl.action   = BOARDIOC_USBDEV_CONNECT;
    ctrl.instance = 0; // CONFIG_NSH_USBDEV_MINOR;
    ctrl.handle   = &handle;

    ret = boardctl(BOARDIOC_USBDEV_CONTROL, (uintptr_t)&ctrl);
    ASSERT_EQUALS_V(OK, ret);

    /* Try to open the console */
    int cnt= 0;
    int fd;
    do {
        fd = open("/dev/ttyACM0", O_RDWR);
        /* ENOTCONN means that the USB device is not yet connected. Anything
         * else is bad.
         */

        if(fd < 0) {
            if(errno == ENOTCONN) {
                printf("Nothing connected to USB port, waiting...\n");
                cnt++;
                if(cnt > 20) {
                    printf("Got tired of waiting for connection\n");
                    FAIL();
                }
            }else{
                printf("Got error: %d\n", errno);
                FAIL();
            }

            sleep(1);
        }

    } while (fd < 0);


    printf("USB CDC connected\n");
    size_t n= write(fd, "Hello USB World\n", 16);
    ASSERT_EQUALS_V(16, n);
    close(fd);
    #endif
}


