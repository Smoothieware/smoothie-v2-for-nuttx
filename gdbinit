# GDB initialization script with commands to make debugging Smoothie-v2 more convenient.
#
# This gdbinit file is placed here in the root folder of the repository so that it can be included from the various
# locations in the source tree from which a user might want to launch rake/make and GDB.
#
# Issuing "help user-defined" command from within GDB will list the commands added by this script.
#
# Initially created by Adam Green on June 8th, 2017.


# Hooking load command to automatically reset the processor once the load is completed.
define hookpost-load
    monitor reset
end



# Command to enable/disable catching of Cortex-M faults as soon as they occur.
define catchfaults
    if ($argc > 0 && $arg0==0)
        set var $catch_faults=0
        set var *(int*)0xE000EDFC &= ~0x7F0
    else
        set var $catch_faults=1
        set var *(int*)0xE000EDFC |= 0x7F0
    end
end

document catchfaults
Instructs Cortex-M processor to stop in GDB if any fault is detected.

User can pass in a parameter of 0 to disable this feature.
end


# Command to display information about an ARMv7-M fault if one is currently active in the current frame.
define showfault
    set var $ipsr_val = $xpsr & 0xF
    if ($ipsr_val >= 3 && $ipsr_val <= 6)
        # Dump Hard Fault.
        set var $fault_reg = *(unsigned int*)0xE000ED2C
        if ($fault_reg != 0)
            printf "**Hard Fault**\n"
            printf "  Status Register: 0x%08X\n", $fault_reg
            if ($fault_reg & (1 << 31))
                printf "    Debug Event\n"
            end
            if ($fault_reg & (1 << 1))
                printf "    Vector Table Read\n"
            end
            if ($fault_reg & (1 << 30))
                printf "    Forced\n"
            end
        end
    
        set var $cfsr_val = *(unsigned int*)0xE000ED28
    
        # Dump Memory Fault.
        set var $fault_reg = $cfsr_val & 0xFF
        if ($fault_reg != 0)
            printf "**MPU Fault**\n"
            printf "  Status Register: 0x%08X\n", $fault_reg
            if ($fault_reg & (1 << 7))
                printf "    Fault Address: 0x%08X\n", *(unsigned int*)0xE000ED34
            end
            if ($fault_reg & (1 << 5))
                printf "    FP Lazy Preservation\n"
            end
            if ($fault_reg & (1 << 4))
                printf "    Stacking Error\n"
            end
            if ($fault_reg & (1 << 3))
                printf "    Unstacking Error\n"
            end
            if ($fault_reg & (1 << 1))
                printf "    Data Access\n"
            end
            if ($fault_reg & (1 << 0))
                printf "    Instruction Fetch\n"
            end
        end
    
        # Dump Bus Fault.
        set var $fault_reg = ($cfsr_val >> 8) & 0xFF
        if ($fault_reg != 0)
            printf "**Bus Fault**\n"
            printf "  Status Register: 0x%08X\n", $fault_reg
            if ($fault_reg & (1 << 7))
                printf "    Fault Address: 0x%08X\n", *(unsigned int*)0xE000ED38
            end
            if ($fault_reg & (1 << 5))
                printf "    FP Lazy Preservation\n"
            end
            if ($fault_reg & (1 << 4))
                printf "    Stacking Error\n"
            end
            if ($fault_reg & (1 << 3))
                printf "    Unstacking Error\n"
            end
            if ($fault_reg & (1 << 2))
                printf "    Imprecise Data Access\n"
            end
            if ($fault_reg & (1 << 1))
                printf "    Precise Data Access\n"
            end
            if ($fault_reg & (1 << 0))
                printf "    Instruction Prefetch\n"
            end
        end
    
        # Usage Fault.
        set var $fault_reg = $cfsr_val >> 16
        if ($fault_reg != 0)
            printf "**Usage Fault**\n"
            printf "  Status Register: 0x%08X\n", $fault_reg
            if ($fault_reg & (1 << 9))
                printf "    Divide by Zero\n"
            end
            if ($fault_reg & (1 << 8))
                printf "    Unaligned Access\n"
            end
            if ($fault_reg & (1 << 3))
                printf "    Coprocessor Access\n"
            end
            if ($fault_reg & (1 << 2))
                printf "    Invalid Exception Return State\n"
            end
            if ($fault_reg & (1 << 1))
                printf "    Invalid State\n"
            end
            if ($fault_reg & (1 << 0))
                printf "    Undefined Instruction\n"
            end
        end
    else
        printf "Not currently in Cortex-M fault handler!\n"
    end
    
end

document showfault
Display ARMv7-M fault information if current stack frame is in a fault handler.
end


# Command to show 'user' mode portion of nuttx stack.
define user
    if (($xpsr & 0xff) != 0 && $stk_swapped == 0)
        # Walk up the stack until we find the exception_common() frame.
        select-frame 0
        set var $stk_found=0
        set var $stk_last_pc=$pc
        while (1)
            # Have GDB go up one level in the stack frame.
            up-silently

            # If the program counter doesn't change then we have already hit the top stack frame without finding the
            # exception_common() frame.
            if ($pc == $stk_last_pc)
                loop_break
            end

            # If you catch the fault early enough then there will be a <signal handler> item on the stack. Can stop at
            # this entry since the user mode stack will still be valid and nuttx hasn't saved context yet.
            if (((unsigned int)$pc & 0xFFFFFF00) == 0xFFFFFF00)
                loop_break
            end

            # Do a few checks to see if this stack frame matches the known pattern for exception_common().
            if (*(unsigned short*)($pc-6) == 0x4695 && $sp == ($r4 & ~7))
                set var $stk_found=1
                loop_break
            end
        end

        if ($stk_found == 0)
            printf "error: Walked to the top stack frame without finding the NuttX 'user' context.\n"
            select-frame 0
        else
            # r4 contains the SP at the time that exception_common finished pushing the non-volatile registers.
            set var $stk_fault_sp=(unsigned int)$r4

            # GDB only allows the context to be switched (setting pc/lr, etc) when in frame 0 so we can switch it back
            # to frame 0 now that we have grabbed the contents of r4.
            select-frame 0

            # Save away the current contents of the kernel registers that we are going to modify shortly.
            set var $stk_swapped_r0=$r0
            set var $stk_swapped_r1=$r1
            set var $stk_swapped_r2=$r2
            set var $stk_swapped_r3=$r3
            set var $stk_swapped_r4=$r4
            set var $stk_swapped_r5=$r5
            set var $stk_swapped_r6=$r6
            set var $stk_swapped_r7=$r7
            set var $stk_swapped_r8=$r8
            set var $stk_swapped_r9=$r9
            set var $stk_swapped_r10=$r10
            set var $stk_swapped_r11=$r11
            set var $stk_swapped_r12=$r12
            set var $stk_swapped_sp=$sp
            set var $stk_swapped_lr=$lr
            set var $stk_swapped_pc=$pc
            set var $stk_swapped_xpsr=$xpsr
            set var $stk_swapped_s0=$s0
            set var $stk_swapped_s1=$s1
            set var $stk_swapped_s2=$s2
            set var $stk_swapped_s3=$s3
            set var $stk_swapped_s4=$s4
            set var $stk_swapped_s5=$s5
            set var $stk_swapped_s6=$s6
            set var $stk_swapped_s7=$s7
            set var $stk_swapped_s8=$s8
            set var $stk_swapped_s9=$s9
            set var $stk_swapped_s10=$s10
            set var $stk_swapped_s11=$s11
            set var $stk_swapped_s12=$s12
            set var $stk_swapped_s13=$s13
            set var $stk_swapped_s14=$s14
            set var $stk_swapped_s15=$s15
            set var $stk_swapped_s16=$s16
            set var $stk_swapped_s17=$s17
            set var $stk_swapped_s18=$s18
            set var $stk_swapped_s19=$s19
            set var $stk_swapped_s20=$s20
            set var $stk_swapped_s21=$s21
            set var $stk_swapped_s22=$s22
            set var $stk_swapped_s23=$s23
            set var $stk_swapped_s24=$s24
            set var $stk_swapped_s25=$s25
            set var $stk_swapped_s26=$s26
            set var $stk_swapped_s27=$s27
            set var $stk_swapped_s28=$s28
            set var $stk_swapped_s29=$s29
            set var $stk_swapped_s30=$s30
            set var $stk_swapped_s31=$s31
            set var $stk_swapped_fpscr=$fpscr

            # Set the registers back to the way they were when the user mode switched into kernel mode.
            # exception_common() calculates user mode SP but we may need to apply 8-byte alignment fixup later before
            # setting $sp to this value. Need $xpsr value to know if this 8-byte alignment fixup is required.
            set var $stk_user_sp=(*(unsigned int*)($stk_fault_sp+0*4))
            # Pull the non-volatile registers that were pushed onto the stack by exception_common().
            set var $r4=*(unsigned int*)($stk_fault_sp+2*4)
            set var $r5=*(unsigned int*)($stk_fault_sp+3*4)
            set var $r6=*(unsigned int*)($stk_fault_sp+4*4)
            set var $r7=*(unsigned int*)($stk_fault_sp+5*4)
            set var $r8=*(unsigned int*)($stk_fault_sp+6*4)
            set var $r9=*(unsigned int*)($stk_fault_sp+7*4)
            set var $r10=*(unsigned int*)($stk_fault_sp+8*4)
            set var $r11=*(unsigned int*)($stk_fault_sp+9*4)
            set var $s16=*(unsigned int*)($stk_fault_sp+11*4)
            set var $s17=*(unsigned int*)($stk_fault_sp+12*4)
            set var $s18=*(unsigned int*)($stk_fault_sp+13*4)
            set var $s19=*(unsigned int*)($stk_fault_sp+14*4)
            set var $s20=*(unsigned int*)($stk_fault_sp+15*4)
            set var $s21=*(unsigned int*)($stk_fault_sp+16*4)
            set var $s22=*(unsigned int*)($stk_fault_sp+17*4)
            set var $s23=*(unsigned int*)($stk_fault_sp+18*4)
            set var $s24=*(unsigned int*)($stk_fault_sp+19*4)
            set var $s25=*(unsigned int*)($stk_fault_sp+20*4)
            set var $s26=*(unsigned int*)($stk_fault_sp+21*4)
            set var $s27=*(unsigned int*)($stk_fault_sp+22*4)
            set var $s28=*(unsigned int*)($stk_fault_sp+23*4)
            set var $s29=*(unsigned int*)($stk_fault_sp+24*4)
            set var $s30=*(unsigned int*)($stk_fault_sp+25*4)
            set var $s31=*(unsigned int*)($stk_fault_sp+26*4)

            # Now pulling the auto-stacked registers.
            set var $r0=*(unsigned int*)($stk_fault_sp+27*4)
            set var $r1=*(unsigned int*)($stk_fault_sp+28*4)
            set var $r2=*(unsigned int*)($stk_fault_sp+29*4)
            set var $r3=*(unsigned int*)($stk_fault_sp+30*4)
            set var $r12=*(unsigned int*)($stk_fault_sp+31*4)
            set var $lr=*(unsigned int*)($stk_fault_sp+32*4)
            set var $pc=*(unsigned int*)($stk_fault_sp+33*4)
            set var $xpsr=*(unsigned int*)($stk_fault_sp+34*4)
            set var $s0=*(float*)($stk_fault_sp+35*4)
            set var $s1=*(float*)($stk_fault_sp+36*4)
            set var $s2=*(float*)($stk_fault_sp+37*4)
            set var $s3=*(float*)($stk_fault_sp+38*4)
            set var $s4=*(float*)($stk_fault_sp+39*4)
            set var $s5=*(float*)($stk_fault_sp+40*4)
            set var $s6=*(float*)($stk_fault_sp+41*4)
            set var $s7=*(float*)($stk_fault_sp+42*4)
            set var $s8=*(float*)($stk_fault_sp+43*4)
            set var $s9=*(float*)($stk_fault_sp+44*4)
            set var $s10=*(float*)($stk_fault_sp+45*4)
            set var $s11=*(float*)($stk_fault_sp+46*4)
            set var $s12=*(float*)($stk_fault_sp+47*4)
            set var $s13=*(float*)($stk_fault_sp+48*4)
            set var $s14=*(float*)($stk_fault_sp+49*4)
            set var $s15=*(float*)($stk_fault_sp+50*4)
            set var $fpscr=*(unsigned int*)($stk_fault_sp+51*4)
            # Apply the 8-byte alignment to the user mode SP if the $xpsr indicates it was done.
            if ($xpsr & (1<<9))
                set var $stk_user_sp=$stk_user_sp+4
                set var $xpsr=$xpsr & ~0x200
            end
            set var $sp=$stk_user_sp

            set var $stk_swapped=1
            bt
        end
    else
        printf "Already showing 'user' mode portion of NuttX stack.\n"
    end
end

document user
Switches to 'user' mode portion of NuttX stack.
end


# Command to show 'kernel' mode portion of nuttx stack.
define kernel
    if ($stk_swapped == 1)
        # GDB only allows the context to be switched (setting pc/lr, etc) when in frame 0.
        select-frame 0

        # Restore the contents of the kernel registers.
        set var $r0=$stk_swapped_r0
        set var $r1=$stk_swapped_r1
        set var $r2=$stk_swapped_r2
        set var $r3=$stk_swapped_r3
        set var $r4=$stk_swapped_r4
        set var $r5=$stk_swapped_r5
        set var $r6=$stk_swapped_r6
        set var $r7=$stk_swapped_r7
        set var $r8=$stk_swapped_r8
        set var $r9=$stk_swapped_r9
        set var $r10=$stk_swapped_r10
        set var $r11=$stk_swapped_r11
        set var $r12=$stk_swapped_r12
        set var $sp=$stk_swapped_sp
        set var $lr=$stk_swapped_lr
        set var $pc=$stk_swapped_pc
        set var $xpsr=$stk_swapped_xpsr
        set var $s0=$stk_swapped_s0
        set var $s1=$stk_swapped_s1
        set var $s2=$stk_swapped_s2
        set var $s3=$stk_swapped_s3
        set var $s4=$stk_swapped_s4
        set var $s5=$stk_swapped_s5
        set var $s6=$stk_swapped_s6
        set var $s7=$stk_swapped_s7
        set var $s8=$stk_swapped_s8
        set var $s9=$stk_swapped_s9
        set var $s10=$stk_swapped_s10
        set var $s11=$stk_swapped_s11
        set var $s12=$stk_swapped_s12
        set var $s13=$stk_swapped_s13
        set var $s14=$stk_swapped_s14
        set var $s15=$stk_swapped_s15
        set var $s16=$stk_swapped_s16
        set var $s17=$stk_swapped_s17
        set var $s18=$stk_swapped_s18
        set var $s19=$stk_swapped_s19
        set var $s20=$stk_swapped_s20
        set var $s21=$stk_swapped_s21
        set var $s22=$stk_swapped_s22
        set var $s23=$stk_swapped_s23
        set var $s24=$stk_swapped_s24
        set var $s25=$stk_swapped_s25
        set var $s26=$stk_swapped_s26
        set var $s27=$stk_swapped_s27
        set var $s28=$stk_swapped_s28
        set var $s29=$stk_swapped_s29
        set var $s30=$stk_swapped_s30
        set var $s31=$stk_swapped_s31
        set var $fpscr=$stk_swapped_fpscr

        set var $stk_swapped=0
        bt
    else
        printf "No saved 'kernel' stack to be restored.\n"
    end
end

document kernel
Switches back to the 'kernel' mode portion of NuttX stack.
end


# Dumps a core dump that is compatible with CrashDebug (https://github.com/adamgreen/CrashDebug).
define gcore
    if ($stk_swapped)
        kernel
    end
    select-frame 0

    # Starts with a header that indicates this is a CrashCatcher dump file.
    dump binary value crash.dump (unsigned int)0x00024363

    # Hardcoding flags to indicate that there will be floating point registers in dump file.
    append binary value crash.dump (unsigned int)0x00000001

    # Dump the integer registers.
    append binary value crash.dump (unsigned int)$r0
    append binary value crash.dump (unsigned int)$r1
    append binary value crash.dump (unsigned int)$r2
    append binary value crash.dump (unsigned int)$r3
    append binary value crash.dump (unsigned int)$r4
    append binary value crash.dump (unsigned int)$r5
    append binary value crash.dump (unsigned int)$r6
    append binary value crash.dump (unsigned int)$r7
    append binary value crash.dump (unsigned int)$r8
    append binary value crash.dump (unsigned int)$r9
    append binary value crash.dump (unsigned int)$r10
    append binary value crash.dump (unsigned int)$r11
    append binary value crash.dump (unsigned int)$r12
    append binary value crash.dump (unsigned int)$sp
    append binary value crash.dump (unsigned int)$lr
    append binary value crash.dump (unsigned int)$pc
    append binary value crash.dump (unsigned int)$xpsr

    # The exception PSR and crashing PSR are one in the same.
    append binary value crash.dump (unsigned int)$xpsr

    # Dump the floating point registers
    append binary value crash.dump (float)$s0
    append binary value crash.dump (float)$s1
    append binary value crash.dump (float)$s2
    append binary value crash.dump (float)$s3
    append binary value crash.dump (float)$s4
    append binary value crash.dump (float)$s5
    append binary value crash.dump (float)$s6
    append binary value crash.dump (float)$s7
    append binary value crash.dump (float)$s8
    append binary value crash.dump (float)$s9
    append binary value crash.dump (float)$s10
    append binary value crash.dump (float)$s11
    append binary value crash.dump (float)$s12
    append binary value crash.dump (float)$s13
    append binary value crash.dump (float)$s14
    append binary value crash.dump (float)$s15
    append binary value crash.dump (float)$s16
    append binary value crash.dump (float)$s17
    append binary value crash.dump (float)$s18
    append binary value crash.dump (float)$s19
    append binary value crash.dump (float)$s20
    append binary value crash.dump (float)$s21
    append binary value crash.dump (float)$s22
    append binary value crash.dump (float)$s23
    append binary value crash.dump (float)$s24
    append binary value crash.dump (float)$s25
    append binary value crash.dump (float)$s26
    append binary value crash.dump (float)$s27
    append binary value crash.dump (float)$s28
    append binary value crash.dump (float)$s29
    append binary value crash.dump (float)$s30
    append binary value crash.dump (float)$s31
    append binary value crash.dump (unsigned int)$fpscr

    # Dump all 128k of RAM starting at 0x10000000.
    #   First two words indicate memory range.
    append binary value crash.dump (unsigned int)0x10000000
    append binary value crash.dump (unsigned int)(0x10000000 + 128*1024)
    append binary memory crash.dump 0x10000000 (0x10000000 + 128*1024)

    # Dump the fault status registers as well.
    append binary value crash.dump (unsigned int)0xE000ED28
    append binary value crash.dump (unsigned int)(0xE000ED28 + 5*4)
    append binary memory crash.dump 0xE000ED28 (0xE000ED28 + 5*4)
end

document gcore
Generate core dump.

The generated core dump can be used with CrashDebug 
(https://github.com/adamgreen/CrashDebug) to reload into GDB at a later point
in time or on another machine. The dump will be generated to a file named
"crash.dump".
end


# Some of the stock GDB commands have to be hooked to properly handle that we may have switched to 'user' mode stack.
# Enable fault catching when user requests execution to be resumed via continue command.
# Also switches back to 'kernel' stack if currently set to 'user' stack.
define hook-continue
    if ($stk_swapped)
        kernel
    end
    if ($catch_faults == 1)
        set var *(int*)0xE000EDFC |= 0x7F0
    end
end

# Switches back to 'kernel' stack if currently set to 'user' stack.
define hook-step
    if ($stk_swapped)
        kernel
    end
end

# Switches back to 'kernel' stack if currently set to 'user' stack.
define hook-monitor
    if ($stk_swapped)
        kernel
    end
end

# Switches back to 'kernel' stack if currently set to 'user' stack.
define hook-quit
    if ($stk_swapped)
        kernel
    end
end



# Commands to run when GDB starts up to get it into a nice state for debugging embedded Cortex-M processors.
set target-charset ASCII
set print pretty on
set mem inaccessible-by-default off

# Default to enabling fault catching when user issues the continue execution command.
set var $catch_faults=1

# This variable is used to track whether we are currently switched to a 'user' mode nuttx stack.
set var $stk_swapped=0
