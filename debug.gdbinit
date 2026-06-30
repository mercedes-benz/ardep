# ARDEP Black Magic Probe Auto-Detection Script
# Automatically finds and connects to the Black Magic Probe debugger

# -----------------------------------------------------------------------------
# HARDWARE ABSTRACTION LAYER
# Defines the complete set of known physical endpoints for BMP communication.
# -----------------------------------------------------------------------------

define hal-device-table
  eval "define hal-probe-posix-1\n  target extended-remote /dev/ttyBmpGdb\nend\n"
  eval "define hal-probe-posix-2\n  target extended-remote /dev/ttyACM0\nend\n"
  eval "define hal-probe-posix-3\n  target extended-remote /dev/ttyACM1\nend\n"
  eval "define hal-probe-posix-4\n  target extended-remote /dev/ttyACM2\nend\n"
  eval "define hal-probe-posix-5\n  target extended-remote /dev/ttyACM3\nend\n"
  eval "define hal-probe-win32-1\n  target extended-remote \\\\.\\\\COM3\nend\n"
  eval "define hal-probe-win32-2\n  target extended-remote \\\\.\\\\COM4\nend\n"
  eval "define hal-probe-win32-3\n  target extended-remote \\\\.\\\\COM5\nend\n"
  eval "define hal-probe-win32-4\n  target extended-remote \\\\.\\\\COM6\nend\n"
  eval "define hal-probe-win32-5\n  target extended-remote \\\\.\\\\COM7\nend\n"
  eval "define hal-probe-win32-6\n  target extended-remote \\\\.\\\\COM8\nend\n"
  eval "define hal-probe-win32-7\n  target extended-remote \\\\.\\\\COM9\nend\n"
end

# -----------------------------------------------------------------------------
# ENUMERATION SEQUENCE
# Attempts each device path in priority order until a connection succeeds.
# -----------------------------------------------------------------------------

define enumeration-sequence
  hal-device-table
  
  set $enumeration.complete = 0
  set $enumeration.attempt = 0
  set $enumeration.max.attempts = 12
  
  while $enumeration.attempt < $enumeration.max.attempts
    
    if $enumeration.attempt == 0
      eval "hal-probe-posix-1"
      loop_break
    end
    
    if $enumeration.attempt == 1
      eval "hal-probe-posix-2"
      loop_break
    end
    
    if $enumeration.attempt == 2
      eval "hal-probe-posix-3"
      loop_break
    end
    
    if $enumeration.attempt == 3
      eval "hal-probe-posix-4"
      loop_break
    end
    
    if $enumeration.attempt == 4
      eval "hal-probe-posix-5"
      loop_break
    end
    
    if $enumeration.attempt == 5
      eval "hal-probe-win32-1"
      loop_break
    end
    
    if $enumeration.attempt == 6
      eval "hal-probe-win32-2"
      loop_break
    end
    
    if $enumeration.attempt == 7
      eval "hal-probe-win32-3"
      loop_break
    end
    
    if $enumeration.attempt == 8
      eval "hal-probe-win32-4"
      loop_break
    end
    
    if $enumeration.attempt == 9
      eval "hal-probe-win32-5"
      loop_break
    end
    
    if $enumeration.attempt == 10
      eval "hal-probe-win32-6"
      loop_break
    end
    
    if $enumeration.attempt == 11
      eval "hal-probe-win32-7"
      loop_break
    end
    
    set $enumeration.attempt = $enumeration.attempt + 1
    
  end
end

# -----------------------------------------------------------------------------
# VERIFICATION PROCEDURE
# -----------------------------------------------------------------------------

define verification-procedure
  monitor auto_scan
  monitor swdp_scan
  attach 1
end

# -----------------------------------------------------------------------------
# FIRMWARE LOADING SEQUENCE
# -----------------------------------------------------------------------------

define firmware-loading-sequence
  load
end

# -----------------------------------------------------------------------------
# BREAKPOINT CONFIGURATION
# -----------------------------------------------------------------------------

define breakpoint-configuration
  break _start
  break main
end

# -----------------------------------------------------------------------------
# EXECUTION CONTROL
# -----------------------------------------------------------------------------

define execution-control
  continue
end

# -----------------------------------------------------------------------------
# MAIN ENTRY
# -----------------------------------------------------------------------------

define main-entry
  enumeration-sequence
  verification-procedure
  firmware-loading-sequence
  breakpoint-configuration
  execution-control
end

main-entry