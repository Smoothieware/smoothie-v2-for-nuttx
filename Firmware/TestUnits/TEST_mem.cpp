#include "../easyunit/test.h"
#include <stdlib.h>
#include <stdio.h>

TEST(MemoryTest,stats)
{
    struct mallinfo mem= mallinfo();
    printf("             total       used       free    largest\n");
    printf("Mem:   %11d%11d%11d%11d\n", mem.arena, mem.uordblks, mem.fordblks, mem.mxordblk);
}

char test_ahb0_ram[100] __attribute__ ((section ("AHBSRAM0")));
char test_ahb1_ram[100] __attribute__ ((section ("AHBSRAM1")));
TEST(MemoryTest,AHBn)
{
    ASSERT_EQUALS_V(0x20000000, (unsigned int)&test_ahb0_ram);
    ASSERT_EQUALS_V(0x20008000, (unsigned int)&test_ahb1_ram);

    for (int i = 0; i < 100; ++i) {
        test_ahb0_ram[i]= i;
        test_ahb1_ram[i]= i+10;
    }

    for (int i = 0; i < 100; ++i) {
        ASSERT_EQUALS_V(i, test_ahb0_ram[i]);
        ASSERT_EQUALS_V(i+10, test_ahb1_ram[i]);
    }
}
