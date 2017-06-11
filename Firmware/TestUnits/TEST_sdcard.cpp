#include "../easyunit/test.h"

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

TEST(SDCardTest, mount)
{
    int ret;
    ret = boardctl(BOARDIOC_INIT, 0);
    ASSERT_EQUALS_V(OK, ret);

    ret = mount(g_source, g_target, g_filesystemtype, 0, nullptr);
    ASSERT_EQUALS_V(0, ret);
}

TEST(SDCardTest, directory)
{
    DIR *dirp;

    /* Open the directory */
    dirp = opendir(g_target);
    ASSERT_TRUE(dirp != NULL);

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
    ASSERT_TRUE(cnt > 0);
}

TEST(SDCardTest, write_read)
{
    char fn[64];
    strcpy(fn, g_target);
    strcat(fn, "/test_file.tst");

    // delete it if it was there
    unlink(fn);

    FILE *fp;
    fp = fopen(fn, "w");
    ASSERT_TRUE(fp != NULL);

    for (int i = 1; i <= 10; ++i) {
        char buf[32];
        int n= snprintf(buf, sizeof(buf), "Line %d\n", i);
        int x= fwrite(buf, 1, n, fp);
        ASSERT_EQUALS_V(n, x);
    }

    fclose(fp);

    // Open file
    fp = fopen(fn, "r");
    ASSERT_TRUE(fp != NULL);

    // check each line of the file
    for (int i = 1; i <= 10; ++i) {
        ASSERT_TRUE(!feof(fp));
        char buf[32];
        char *l= fgets(buf, sizeof(buf), fp);
        ASSERT_TRUE(l != NULL);
        printf("test: %s", buf);
        // now verify
        char vbuf[32];
        int n= snprintf(vbuf, sizeof(vbuf), "Line %d\n", i);
        ASSERT_EQUALS_V(0, strncmp(buf, vbuf, n));
    }
    fclose(fp);
}

TEST(SDCardTest,unmount)
{
    int ret = umount(g_target);
    ASSERT_EQUALS_V(0, ret);
}
