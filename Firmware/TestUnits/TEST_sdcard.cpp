#include "../easyunit/test.h"

#include <sys/mount.h>
#include <sys/types.h>

static const char g_mntdir[]         = "/mnt";
static const char g_target[]         = "/mnt/fs";
static const char g_filesystemtype[] = "vfat";
static const char g_source[]         = "/dev/mmcsd0";

TEST(SDCardTest,mount)
{
    int ret;
    ret = mount(g_source, g_target, g_filesystemtype, 0, nullptr);
    ASSERT_EQUALS_V(0, ret);
}
