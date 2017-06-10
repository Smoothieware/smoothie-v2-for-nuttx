#include "../easyunit/test.h"

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/boardctl.h>

#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>

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
    if (dirp == NULL) {
        /* Failed to open the directory */

        printf("opendir faaild: %d", errno);
        FAIL();
    }

    /* Read each directory entry */
    for (; ; ) {
        struct dirent *entryp = readdir(dirp);
        if (entryp == NULL) {
            /* Finished with this directory */
            break;
        }

        printf("%s\n", entryp->d_name);
    }

    closedir(dirp);
}

TEST(SDCardTest, read)
{
    const char *fn= "/mnt/fs/config";

    // Open file
    FILE *lp = fopen(fn, "r");
    if (lp == NULL) {
        printf("File not found: %s\r\n", fn);
        FAIL();
    }

    // Print each line of the file
    char buf[132];
    while (!feof(lp)) {
        char *l= fgets(buf, sizeof(buf), lp);
        if(l != NULL) {
            printf("%s\n", buf);
        }
    };
    fclose(lp);

}
