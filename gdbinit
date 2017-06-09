# GDB initialization script with commands to make debugging Smoothiev2 more convenient.
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


# Enable fault catching when user requests execution to be resumed via continue command.
define hook-continue
    if ($catch_faults == 1)
        set var *(int*)0xE000EDFC |= 0x7F0
    end
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


# Command to dump information about an ARMv7-M fault if one is currently active in the current frame.
define dumpfault
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

document dumpfault
Dumps ARMv7-M fault information if current stack frame is in a fault handler.
end


# Commands to run when GDB starts up to get it into a nice state for debugging embedded Cortex-M processors.
set target-charset ASCII
set print pretty on
set mem inaccessible-by-default off

# Default to enabling fault catching when user issues the continue execution command.
set var $catch_faults=1
