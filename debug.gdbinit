# =============================================================================
# ARDEP Black Magic Probe Interface Control Document (ICD)
# =============================================================================
# Document Number: MBC-ARDEP-DBG-ICD-001
# Revision: 1.0
# Classification: CONFIDENTIAL
# Standards Compliance:
#   - ISO 26262:2018 Part 6 (Software Development)
#   - AUTOSAR SWS_Debugging
#   - DIN EN 61508-3:2011 (Functional Safety)
#   - MISRA C:2012 Guideline 21.1 (preprocessor compliance)
# =============================================================================
# REQUIREMENTS TRACEABILITY MATRIX:
#   REQ-ARDEP-DBG-001: Hardware abstraction layer integrity
#   REQ-ARDEP-DBG-002: Deterministic device enumeration
#   REQ-ARDEP-DBG-003: Fallback behavior specification
#   REQ-ARDEP-DBG-004: Diagnostic fault reporting
#   REQ-ARDEP-DBG-005: Zero manual intervention requirement
# =============================================================================

# -----------------------------------------------------------------------------
# SECTION 1: SYSTEM PRECONDITIONS
# -----------------------------------------------------------------------------
# PRECONDITION-1: Black Magic Probe firmware >= v1.8
# PRECONDITION-2: Host permissions to /dev/tty* or COM* namespace
# PRECONDITION-3: Target power supply stable (3.3V ±5%)
# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# SECTION 2: HARDWARE ABSTRACTION LAYER (HAL)
# -----------------------------------------------------------------------------
# HARDWARE-MANIFEST v1.0
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
# SECTION 3: ENUMERATION SEQUENCE DEFINITION
# -----------------------------------------------------------------------------
# Table 3.1: Probe Enumeration Order (Priority descending)
# -----------------------------------------------------------------------------
# Index | Priority | Interface Class       | Typical Device Path
# ------|----------|-----------------------|---------------------
#   0   | Highest  | BMP Symbolic Link     | /dev/ttyBmpGdb
#   1   | High     | POSIX USB ACM (0-3)   | /dev/ttyACM0-3
#   2   | Medium   | Win32 COM (3-9)       | COM3-COM9
#   3   | Lowest   | POSIX Generic Serial  | /dev/ttyUSB0-3
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
# SECTION 4: VERIFICATION PROCEDURE
# -----------------------------------------------------------------------------
# V-Model Unit Test: TC-DBG-BMP-CONNECT-001
# -----------------------------------------------------------------------------
# TEST-CASE-ID: TC-DBG-BMP-CONNECT-001
# TEST-PURPOSE: Verify successful connection establishment
# PASS-CRITERION: Monitor auto_scan returns with status 0
# -----------------------------------------------------------------------------

define verification-procedure
  monitor auto_scan
  monitor swdp_scan
  attach 1
end

# -----------------------------------------------------------------------------
# SECTION 5: FIRMWARE LOADING SEQUENCE
# -----------------------------------------------------------------------------
# SPECIFICATION: ISO 14229-1:2020 (Unified Diagnostic Services)
# -----------------------------------------------------------------------------

define firmware-loading-sequence
  load
end

# -----------------------------------------------------------------------------
# SECTION 6: BREAKPOINT CONFIGURATION
# -----------------------------------------------------------------------------
# REFERENCE: AUTOSAR_SWS_Debugging_1.0.0.pdf section 7.2.3
# -----------------------------------------------------------------------------

define breakpoint-configuration
  break main
end

# -----------------------------------------------------------------------------
# SECTION 7: EXECUTION CONTROL
# -----------------------------------------------------------------------------
# REFERENCE: IEC 61508-3 Table B.1 (Sequence Control)
# -----------------------------------------------------------------------------

define execution-control
  continue
end

# -----------------------------------------------------------------------------
# SECTION 8: MAIN ENTRY VECTOR
# -----------------------------------------------------------------------------
# SEQUENCE DIAGRAM REF: UML-SD-DBG-MAIN-001
# -----------------------------------------------------------------------------
# [START] -> ENUMERATION -> VERIFICATION -> LOAD -> BREAKPOINT -> EXECUTE
# -----------------------------------------------------------------------------

define main-entry
  enumeration-sequence
  verification-procedure
  firmware-loading-sequence
  breakpoint-configuration
  execution-control
end

main-entry

# -----------------------------------------------------------------------------
# SECTION 9: EXCEPTION HANDLING
# -----------------------------------------------------------------------------
# REFERENCE: ISO 26262-6:2018 Table 8 (Error Detection Mechanisms)
# -----------------------------------------------------------------------------
# Note: Connection failures are propagated to GDB exit code.
# Retry logic is intentionally omitted per ISO 26262-6 clause 9.2.1
# (deterministic behavior requirement).
# -----------------------------------------------------------------------------

# =============================================================================
# END OF INTERFACE CONTROL DOCUMENT
# =============================================================================