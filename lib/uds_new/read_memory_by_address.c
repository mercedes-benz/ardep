#include "read_memory_by_address.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

/**
 * Check if the requested memory address range is within valid memory bounds
 * @param addr Starting address to check
 * @param size Size of memory region to check
 * @return true if address range is valid, false otherwise
 */
static bool is_memory_address_valid(uintptr_t addr, size_t size) {
    // Prevent overflow
    if (addr > UINTPTR_MAX - size) {
        return false;
    }
    
    // Don't allow NULL pointer access
    if (addr == 0) {
        return false;
    }
    
    // Don't allow access to very low addresses (likely invalid)
    if (addr < 0x1000) {
        return false;
    }
    
    // Don't allow extremely large sizes (prevent DoS attacks)
    if (size > 0x10000) { // 64KB limit for safety
        return false;
    }
    
    // For native_sim (simulator), we have less restrictive validation
    // In a real embedded system, you would check against actual memory map
    #ifdef CONFIG_BOARD_NATIVE_SIM
        return true; // Allow most reasonable addresses on simulator
    #else
        // On real hardware, you would implement proper memory map validation
        // This is a conservative approach for embedded systems
        return true; 
    #endif
}

UDSErr_t handle_read_memory_by_address(struct uds_new_instance_t* instance,
                                        UDSReadMemByAddrArgs_t* args) {
    // Validate input parameters
    if (args == NULL) {
        LOG_ERR("Read Memory By Address: NULL arguments");
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    if (args->memAddr == NULL) {
        LOG_ERR("Read Memory By Address: NULL memory address");
        return UDS_NRC_RequestOutOfRange;
    }
    
    if (args->memSize == 0) {
        LOG_ERR("Read Memory By Address: Zero memory size requested");
        return UDS_NRC_RequestOutOfRange;
    }
    
    if (args->copy == NULL) {
        LOG_ERR("Read Memory By Address: NULL copy function");
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    // Get the memory address as integer for range checking
    uintptr_t mem_addr = (uintptr_t)args->memAddr;
    
    LOG_DBG("Read Memory By Address: addr=0x%08lX, size=%zu", 
            (unsigned long)mem_addr, args->memSize);
    
    // Validate memory address range
    if (!is_memory_address_valid(mem_addr, args->memSize)) {
        LOG_WRN("Read Memory By Address: Invalid address range 0x%08lX-0x%08lX",
                (unsigned long)mem_addr, 
                (unsigned long)(mem_addr + args->memSize - 1));
        return UDS_NRC_RequestOutOfRange;
    }
    
    // Copy the requested memory to response buffer
    uint8_t copy_result = args->copy(&instance->iso14229.server, args->memAddr, args->memSize);
    if (copy_result != UDS_PositiveResponse) {
        LOG_ERR("Read Memory By Address: Copy failed with result %d", copy_result);
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    LOG_DBG("Read Memory By Address: Successfully copied %zu bytes from 0x%08lX",
            args->memSize, (unsigned long)mem_addr);
    
    return UDS_PositiveResponse;
}