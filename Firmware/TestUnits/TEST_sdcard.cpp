#include "../Unity/src/unity.h"
#include "TestRegistry.h"

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/boardctl.h>

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char g_mntdir[]         = "/mnt";
static const char g_target[]         = "/mnt/fs";
static const char g_filesystemtype[] = "vfat";
static const char g_source[]         = "/dev/mmcsd0";

REGISTER_TEST(SDCardTest, mount)
{
    int ret;
    // ret = boardctl(BOARDIOC_INIT, 0);
    // TEST_ASSERT_EQUAL_INT(OK, ret);

    ret = mount(g_source, g_target, g_filesystemtype, 0, nullptr);
    TEST_ASSERT_EQUAL_INT(0, ret);
}


REGISTER_TEST(SDCardTest, write_read)
{
    char fn[64];
    strcpy(fn, g_target);
    strcat(fn, "/test_file.tst");

    // delete it if it was there
    unlink(fn);

    FILE *fp;
    fp = fopen(fn, "w");
    TEST_ASSERT_NOT_NULL(fp);

    for (int i = 1; i <= 10; ++i) {
        char buf[32];
        int n= snprintf(buf, sizeof(buf), "Line %d\n", i);
        int x= fwrite(buf, 1, n, fp);
        TEST_ASSERT_EQUAL_INT(n, x);
    }

    fclose(fp);

    // Open file
    fp = fopen(fn, "r");
    TEST_ASSERT_NOT_NULL(fp);

    // check each line of the file
    for (int i = 1; i <= 10; ++i) {
        TEST_ASSERT_TRUE(!feof(fp));
        char buf[32];
        char *l= fgets(buf, sizeof(buf), fp);
        TEST_ASSERT_NOT_NULL(l);
        printf("test: %s", buf);
        // now verify
        char vbuf[32];
        int n= snprintf(vbuf, sizeof(vbuf), "Line %d\n", i);
        TEST_ASSERT_EQUAL_INT(0, strncmp(buf, vbuf, n));
    }
    fclose(fp);
}

REGISTER_TEST(SDCardTest, directory)
{
    DIR *dirp;

    /* Open the directory */
    dirp = opendir(g_target);
    TEST_ASSERT_NOT_NULL(dirp);

    /* Read each directory entry */
    int cnt= 0;
    for (; ; ) {
        struct dirent *entryp = readdir(dirp);
        if (entryp == NULL) {
            /* Finished with this directory */
            break;
        }

        printf("%s\n", entryp->d_name);
        cnt++;
    }
    closedir(dirp);
    TEST_ASSERT_TRUE(cnt > 0);
}

REGISTER_TEST(SDCardTest,read_config_init)
{
    TEST_IGNORE();
}


REGISTER_TEST(SDCardTest, time_read_write)
{
    char fn[64];
    strcpy(fn, g_target);
    strcat(fn, "/test_large_file.tst");

    // delete it if it was there
    unlink(fn);

    FILE *fp;
    fp = fopen(fn, "w");
    TEST_ASSERT_NOT_NULL(fp);

    systime_t st = clock_systimer();

    uint32_t n= 5000;
    for (uint32_t i = 1; i <= n; ++i) {
        char buf[512];
        size_t x= fwrite(buf, 1, sizeof(buf), fp);
        if(x != sizeof(buf)) {
            TEST_FAIL();
        }
    }

    systime_t en = clock_systimer();
    printf("elapsed time %d us for writing %d bytes, %1.4f bytes/sec\n", TICK2USEC(en-st), n*512, (n*512.0F) / (TICK2USEC(en-st)/1e6F));

    fclose(fp);

    // Open file
    fp = fopen(fn, "r");
    TEST_ASSERT_NOT_NULL(fp);

    // read back data
    st = clock_systimer();
    for (uint32_t i = 1; i <= n; ++i) {
        char buf[512];
        size_t x= fread(buf, 1, sizeof(buf), fp);
        if(x != sizeof(buf)) {
            TEST_FAIL();
        }
    }
    en = clock_systimer();
    printf("elapsed time %d us for reading %d bytes, %1.4f bytes/sec\n", TICK2USEC(en-st), n*512, (n*512.0F) / (TICK2USEC(en-st)/1e6F));

    fclose(fp);
}

REGISTER_TEST(SDCardTest,unmount)
{
    int ret = umount(g_target);
    TEST_ASSERT_EQUAL_INT(0, ret);
}
