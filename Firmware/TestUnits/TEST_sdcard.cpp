#include "../easyunit/test.h"

#include <sys/mount.h>

static const char g_mntdir[]         = "/mnt";
static const char g_target[]         = "/mnt/fs";
static const char g_filesystemtype[] = "vfat";
static const char g_source[]         = "/dev/sdio";

TEST(SDCardTest,mount)
{
    int ret;
    ret = mount(g_source, g_target, g_filesystemtype, 0, nullptr);
    ASSERT_EQUALS_V(0, ret);
}
