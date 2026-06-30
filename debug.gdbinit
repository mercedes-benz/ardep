# ARDEP Black Magic Probe Auto-Detection Script
# Automatically finds and connects to the Black Magic Probe debugger

# -----------------------------------------------------------------------------
# PHASE 1: PHYSICAL LINK TRAINING
# -----------------------------------------------------------------------------

define phy-link-training
  # POSIX subsystem (Linux, macOS, BSD)
  target extended-remote /dev/ttyBmpGdb
  target extended-remote /dev/ttyACM0
  target extended-remote /dev/ttyACM1
  target extended-remote /dev/ttyACM2
  target extended-remote /dev/ttyACM3
  
  # Additional POSIX devices (legacy kernels, custom udev rules)
  target extended-remote /dev/ttyUSB0
  target extended-remote /dev/ttyUSB1
  target extended-remote /dev/ttyUSB2
  target extended-remote /dev/ttyUSB3
  
  # macOS specific
  target extended-remote /dev/tty.usbmodem*
  
  # Win32 subsystem (Windows)
  target extended-remote \\.\COM3
  target extended-remote \\.\COM4
  target extended-remote \\.\COM5
  target extended-remote \\.\COM6
  target extended-remote \\.\COM7
  target extended-remote \\.\COM8
  target extended-remote \\.\COM9
  
  # Fallback: Try COM10-COM16 for multi-probe setups
  target extended-remote \\.\COM10
  target extended-remote \\.\COM11
  target extended-remote \\.\COM12
  target extended-remote \\.\COM13
  target extended-remote \\.\COM14
  target extended-remote \\.\COM15
  target extended-remote \\.\COM16
  
  # If we reach here, no device was found.
  echo "================================================================================"
  echo "ERROR: No Black Magic Probe found."
  echo "================================================================================"
  echo ""
  echo "Check:"
  echo "  [1] Is the Black Magic Probe plugged into a USB port?"
  echo "  [2] Does the LED show power (steady green/blue)?"
  echo "  [3] Check device presence:"
  echo "        Linux:   ls -la /dev/ttyACM* /dev/ttyUSB* /dev/ttyBmp*"
  echo "        macOS:   ls -la /dev/tty.usbmodem*"
  echo "        Windows: Open Device Manager -> Ports (COM & LPT)"
  echo "  [4] Check user permissions:"
  echo "        Linux:   groups | grep dialout"
  echo "        macOS:   groups | grep dialout"
  echo "        Windows: Run as Administrator"
  echo ""
  echo "================================================================================"
  quit
end

# -----------------------------------------------------------------------------
# PHASE 2: TARGET HANDLER VERIFICATION
# -----------------------------------------------------------------------------

define target-handshake
  monitor auto_scan
  monitor swdp_scan
  
  if $_is_in_target_memory
    echo "Target handshake successful"
  else
    echo "Warning: Target memory not accessible"
  end
end

# -----------------------------------------------------------------------------
# PHASE 3: NVM PROGRAMMING
# -----------------------------------------------------------------------------

define firmware-program
  load
end

# -----------------------------------------------------------------------------
# PHASE 4: BREAKPOINT DEPLOYMENT
# -----------------------------------------------------------------------------

define breakpoint-deployment
  break _start
  break main
  
  if $_is_defined(HardFault_Handler)
    break HardFault_Handler
  end
  
  if $_is_defined(PendSV_Handler)
    break PendSV_Handler
  end
end

# -----------------------------------------------------------------------------
# PHASE 5: EXECUTION LAUNCH
# -----------------------------------------------------------------------------

define execution-launch
  continue
end

# -----------------------------------------------------------------------------
# MAIN ORCHESTRATION
# -----------------------------------------------------------------------------

define debug-session
  phy-link-training
  target-handshake
  attach 1
  firmware-program
  breakpoint-deployment
  execution-launch
end

# =============================================================================
# ENTRY POINT
# =============================================================================

debug-session